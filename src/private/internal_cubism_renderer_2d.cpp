// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 MizunagiKB <mizukb@live.jp>
// ----------------------------------------------------------------- include(s)
#include <gd_cubism.hpp>

#include <CubismFramework.hpp>
#include <Model/CubismModel.hpp>
#include <Rendering/CubismRenderer.hpp>

#include <private/internal_cubism_renderer_2d.hpp>
#include <cfloat>

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <cfloat>

// ------------------------------------------------------------------ define(s)
// --------------------------------------------------------------- namespace(s)
using namespace Live2D::Cubism::Core;
using namespace Live2D::Cubism::Framework;
using namespace Live2D::Cubism::Framework::Rendering;
using namespace godot;

// -------------------------------------------------------------------- enum(s)
// ------------------------------------------------------------------- const(s)
// ------------------------------------------------------------------ static(s)
PackedInt32Array make_Indices(const csmUint16 *ptr, const int32_t &size);
PackedVector2Array make_UVs(const Live2D::Cubism::Core::csmVector2 *ptr, const int32_t &size);
PackedVector2Array make_Vertices(const Live2D::Cubism::Core::csmVector2 *ptr, const int32_t &size, const Csm::csmFloat32 &ppunit);
const Vector4 make_vector4(const Live2D::Cubism::Core::csmVector4 &src_vec4);

// ----------------------------------------------------------- class:forward(s)
// ------------------------------------------------------------------- class(s)
void InternalCubismRenderer2D::update_material(const Csm::CubismModel *model, const Csm::csmInt32 index, MeshInstance2D *mesh)
{
    mesh->set_instance_shader_parameter("color_base", Vector4(1.0, 1.0, 1.0, model->GetDrawableOpacity(index)));
    mesh->set_instance_shader_parameter("color_screen", make_vector4(model->GetDrawableScreenColor(index)));
    mesh->set_instance_shader_parameter("color_multiply", make_vector4(model->GetDrawableMultiplyColor(index)));
}

void InternalCubismRenderer2D::update_mesh(
    const Csm::CubismModel *model,
    const Csm::csmInt32 index,
    const Ref<ArrayMesh> ary_mesh,
    const float pp_unit
)
{
    if (ary_mesh->get_surface_count() > 0) {
        const int size = model->GetDrawableVertexCount(index);
        const auto ptr = model->GetDrawableVertexPositions(index);

        PackedByteArray ary;
        ary.resize(size * sizeof(Vector2));

        Vector3 vct_min(DBL_MAX, DBL_MAX, 0.0);
        Vector3 vct_max(DBL_MIN, DBL_MIN, 0.0);

        for (int i = 0, n = 0; i < size; i++, n += sizeof(Vector2))
        {
            float x = ptr[i].X * pp_unit;
            float y = ptr[i].Y * pp_unit;
            vct_min.x = Math::min(vct_min.x, x); // left
            vct_min.y = Math::min(vct_min.y, y); // top
            vct_max.x = Math::max(vct_max.x, x); // right
            vct_max.y = Math::max(vct_max.y, y); // bottom
            
            ary.encode_float(n + offsetof(Vector2, x), x);
            ary.encode_float(n + offsetof(Vector2, y), -y);
        }

        ary_mesh->surface_update_vertex_region(0, 0, ary);

        // aabb does not get automatically updated when directly updating the vertex region
        AABB aabb(Vector3(vct_min.x, -vct_max.y, 0), vct_max - vct_min);
        ary_mesh->set_custom_aabb(aabb);

        return;
    }

    Array ary;
    ary.resize(Mesh::ARRAY_MAX);

    ary[Mesh::ARRAY_VERTEX] = make_Vertices(
        model->GetDrawableVertexPositions(index),
        model->GetDrawableVertexCount(index),
        pp_unit);

    ary[Mesh::ARRAY_TEX_UV] = make_UVs(
        model->GetDrawableVertexUvs(index),
        model->GetDrawableVertexCount(index));

    ary[Mesh::ARRAY_INDEX] = make_Indices(
        model->GetDrawableVertexIndices(index),
        model->GetDrawableVertexIndexCount(index));

    ary_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, ary);
    ary_mesh->set_custom_aabb(ary_mesh->get_aabb());
}

void InternalCubismRenderer2D::update(const CubismModel *model, Array meshes, Array masks, const float ppunit)
{
    const Csm::csmInt32 *renderOrder = model->GetDrawableRenderOrders();

    if (meshes.is_empty()) return;
    
    // get the model's global transform to preform optimizations against
    const auto mesh_0 = Object::cast_to<MeshInstance2D>(meshes[0]);
    
    const Vector2 model_scale = Math::min(mesh_0->get_global_scale(), Vector2(1.0, 1.0));
    const Transform2D viewport_transform = mesh_0->get_global_transform_with_canvas();
    const Rect2 viewport_bounds = mesh_0->get_viewport_rect();

    for (int i = 0; i < meshes.size(); i++)
    {
        MeshInstance2D *node = Object::cast_to<MeshInstance2D>(meshes[i]);
        if (node == nullptr) continue;
        
        int32_t index = node->get_meta("index", -1);
        if (index < 0) continue;

        const bool visible = model->GetDrawableDynamicFlagIsVisible(index) && model->GetDrawableOpacity(index) > 0.0f;
        node->set_visible(visible);
        Ref<ArrayMesh> ary_mesh = node->get_mesh();

        InternalCubismRenderer2D::update_mesh(model, index, ary_mesh, ppunit);
        InternalCubismRenderer2D::update_material(model, index, node);
        node->set_z_index(renderOrder[index]);

        // adjust real bounds to prevent the mesh being culled
        AABB bounds = ary_mesh->get_custom_aabb();
        Rect2 canvas_bounds = Rect2(bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
        RenderingServer::get_singleton()->canvas_item_set_custom_rect(
            node->get_canvas_item(), true,
            canvas_bounds
        );
    }

    real_t frame_time = Time::get_singleton()->get_ticks_msec();

    for (int i = 0; i < masks.size(); i++)
    {
        Node2D *mask = Object::cast_to<Node2D>(masks[i]);
        Vector2 tex_size = Object::cast_to<SubViewport>(mask->get_parent())->get_size();
        Vector4 layout = mask->get_meta("layout_bounds", Vector4(0.0, 0.0, 2.0, 2.0));
        Rect2 layout_bounds(
            Vector2(layout.x, layout.y) * tex_size,
            Vector2(layout.z, layout.w) * tex_size
        );

        Ref<ArrayMesh> mesh = Object::cast_to<MeshInstance2D>(mask->get_child(0))->get_mesh();
        AABB aabb = mesh->get_custom_aabb();

        for (int n = 1; n < mask->get_child_count(); n++) {
            Ref<ArrayMesh> mesh = Object::cast_to<MeshInstance2D>(mask->get_child(n))->get_mesh();
            aabb = aabb.merge(mesh->get_custom_aabb());
        }
        //aabb = aabb.grow(4); // adds padding around the mask for safety

        Rect2 bounds(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

        // figure out the scale required to fit the mask into its cell
        Vector2 mask_size = bounds.size;
        Vector2 cell_size = layout_bounds.size;
        Transform2D target_bounds = Transform2D(0, -bounds.position);
        
        // scale to fit maintaining aspect ratio
        if (mask_size.x > cell_size.x || mask_size.y > cell_size.y) {
            auto w_ratio = cell_size.x / mask_size.x;
            auto h_ratio = cell_size.y / mask_size.y;
            auto ratio = Math::min(w_ratio, h_ratio);
            Vector2 scalar = Vector2(ratio, ratio);
            target_bounds = target_bounds.scaled(scalar);
        }
        target_bounds = target_bounds.translated(layout_bounds.position);
        mask->set_transform(target_bounds);
        
        Array dependent_meshes = mask->get_meta("meshes");
        for (int n = 0; n < dependent_meshes.size(); n++) {
            MeshInstance2D *mesh = Object::cast_to<MeshInstance2D>(mask->get_node_or_null(dependent_meshes[n]));

            mesh->set_instance_shader_parameter("mask_rect", Vector4(bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y));
            mesh->set_instance_shader_parameter("mask_scale", target_bounds.get_scale());
        }
    }
}

// ------------------------------------------------------------------ method(s)
PackedInt32Array make_Indices(const csmUint16 *ptr, const int32_t &size)
{
    PackedInt32Array ary;
    ary.resize(size);
    for (int i = 0; i < size; i++)
    {
        ary.set(i, ptr[i]);
    }
    return ary;
}

PackedVector2Array make_UVs(const Live2D::Cubism::Core::csmVector2 *ptr, const int32_t &size)
{
    PackedVector2Array ary;
    ary.resize(size);
    for (int i = 0; i < size; i++)
    {
        ary.set(i, Vector2(ptr[i].X, 1.0 - ptr[i].Y));
    }
    return ary;
}

PackedVector2Array make_Vertices(const Live2D::Cubism::Core::csmVector2 *ptr, const int32_t &size, const Csm::csmFloat32 &ppunit)
{
    PackedVector2Array ary;
    ary.resize(size);
    for (int i = 0; i < size; i++)
    {
        ary.set(i, Vector2(ptr[i].X, ptr[i].Y * -1.0) * ppunit);
    }
    return ary;
}

const Vector4 make_vector4(const Live2D::Cubism::Core::csmVector4 &src_vec4)
{
    return Vector4(src_vec4.X, src_vec4.Y, src_vec4.Z, src_vec4.W);
}
