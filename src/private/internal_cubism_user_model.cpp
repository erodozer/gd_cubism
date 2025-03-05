// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 MizunagiKB <mizukb@live.jp>
// ----------------------------------------------------------------- include(s)
#include <gd_cubism.hpp>
#include <CubismModelSettingJson.hpp>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#ifdef GD_CUBISM_USE_RENDERER_2D
    #include <private/internal_cubism_renderer_2d.hpp>
#else
    #include <private/internal_cubism_renderer_3d.hpp>
#endif // GD_CUBISM_USE_RENDERER_2D
#include <private/internal_cubism_user_model.hpp>
#include <gd_cubism_user_model.hpp>
#include <gd_cubism_effect.hpp>

// ------------------------------------------------------------------ define(s)
// --------------------------------------------------------------- namespace(s)
using namespace Live2D::Cubism::Framework;


// -------------------------------------------------------------------- enum(s)
// ------------------------------------------------------------------- const(s)
// ------------------------------------------------------------------ static(s)
// ----------------------------------------------------------- class:forward(s)
// ------------------------------------------------------------------- class(s)
InternalCubismUserModel::InternalCubismUserModel(GDCubismUserModel *owner_viewport)
    : CubismUserModel()
    , _owner_viewport(owner_viewport) {

    _debugMode = false;
}


InternalCubismUserModel::~InternalCubismUserModel() {
    this->clear();
}


bool InternalCubismUserModel::model_bind() {
    String _model_pathname = this->_owner_viewport->get_scene_file_path();
    String _model_dir = _model_pathname.get_base_dir();
    this->_updating = true;
    this->_initialized = false;
    
    PackedByteArray buffer = FileAccess::get_file_as_bytes(_model_pathname);
    if(buffer.size() == 0) return false;

    ICubismModelSetting *model_settings = CSM_NEW CubismModelSettingJson(buffer.ptr(), buffer.size());

    // setup Live2D model
    {
        String moc3_pathname = _model_dir.path_join(model_settings->GetModelFileName());
        PackedByteArray buffer = FileAccess::get_file_as_bytes(moc3_pathname);
        this->LoadModel(buffer.ptr(), buffer.size());

        if(this->_modelMatrix == nullptr) {
            this->clear();
            return false;
        }

        this->_model->SaveParameters();
    }
    // Physics
    {
        String path = model_settings->GetPhysicsFileName();
        if (!path.is_empty()) {
            PackedByteArray buffer = FileAccess::get_file_as_bytes(_model_dir.path_join(path));
            if(buffer.size() > 0) {
                this->LoadPhysics(buffer.ptr(), buffer.size());
            }
        }
    }
    // Pose
    {
        String path = model_settings->GetPoseFileName();
        if (!path.is_empty()) {
            PackedByteArray buffer = FileAccess::get_file_as_bytes(_model_dir.path_join(path));
            if(buffer.size() > 0) {
                this->LoadPose(buffer.ptr(), buffer.size());
            }
        }
    }
    //UserData
    {
        String path = model_settings->GetUserDataFile();
        if (!path.is_empty()) {
            PackedByteArray buffer = FileAccess::get_file_as_bytes(_model_dir.path_join(path));
            if(buffer.size() > 0) {
                this->LoadUserData(buffer.ptr(), buffer.size());
            }
        }
    }

    // EyeBlink(Parameters)
    {
        for (uint32_t i = 0; i < model_settings->GetEyeBlinkParameterCount(); i++) {
            this->_list_eye_blink.append(model_settings->GetEyeBlinkParameterId(i));
        }
    }

    // LipSync(Parameters)
    {
        for (uint32_t i = 0; i < model_settings->GetLipSyncParameterCount(); i++) {
            this->_list_lipsync.append(model_settings->GetLipSyncParameterId(i));
        }
    }

    // Hit Areas
    {
        for (uint32_t i = 0; i < model_settings->GetHitAreasCount(); i++) {
            Dictionary dict_hit_area;
            
            dict_hit_area["id"] = model_settings->GetHitAreaId(i);
            dict_hit_area["name"] = model_settings->GetHitAreaName(i);
            ary_hit_areas.append(dict_hit_area);
        }
    }

    this->model_settings = model_settings;
    
    this->CreateRenderer();
		
    this->_updating = false;
    this->_initialized = true;

    return true;
}

void InternalCubismUserModel::pro_update(const float delta) {
    if(this->IsInitialized() == false) return;

    this->effect_batch(delta, EFFECT_CALL_PROLOGUE);

    this->_model->GetModelOpacity();
}


void InternalCubismUserModel::efx_update(const float delta) {
    if(this->IsInitialized() == false) return;

    if(this->_owner_viewport->check_cubism_effect_dirty() == true) {
        this->effect_term();
        this->effect_init();
        this->_owner_viewport->cubism_effect_dirty_reset();
    }

    this->effect_batch(delta, EFFECT_CALL_PROCESS);
}

void InternalCubismUserModel::epi_update(const float delta) {
    if(this->IsInitialized() == false) return;

    if(this->_owner_viewport->physics_evaluate == true) {
        if(this->_physics != nullptr) { this->_physics->Evaluate(this->_model, delta); }
    }

    if(this->_owner_viewport->pose_update == true) {
        if(this->_pose != nullptr) { this->_pose->UpdateParameters(this->_model, delta); }
    }

    this->_model->Update();
    this->effect_batch(delta, EFFECT_CALL_EPILOGUE);
}


void InternalCubismUserModel::update_node() {
    if(this->IsInitialized() == false) return;

    #ifdef GD_CUBISM_USE_RENDERER_2D
    InternalCubismRenderer2D* renderer = this->GetRenderer<InternalCubismRenderer2D>();
    #else
    #endif // GD_CUBISM_USE_RENDERER_2D

    renderer->IsPremultipliedAlpha(false);
    renderer->DrawModel();
    renderer->update(this->_owner_viewport->ary_meshes, this->_owner_viewport->mask_viewport_size);
}


void InternalCubismUserModel::clear() {

    this->DeleteRenderer();

    this->effect_term();

    this->_list_eye_blink.clear();
    this->_list_lipsync.clear();
}


void InternalCubismUserModel::effect_init() {
    for(
        Csm::csmVector<GDCubismEffect*>::iterator i = this->_owner_viewport->_list_cubism_effect.Begin();
        i != this->_owner_viewport->_list_cubism_effect.End();
        i++
    ) {
        (*i)->_cubism_init(this);
    }
}


void InternalCubismUserModel::effect_term() {
    for(
        Csm::csmVector<GDCubismEffect*>::iterator i = this->_owner_viewport->_list_cubism_effect.Begin();
        i != this->_owner_viewport->_list_cubism_effect.End();
        i++
    ) {
        (*i)->_cubism_term(this);
    }
}


void InternalCubismUserModel::effect_batch(const float delta, const EFFECT_CALL efx_call) {
    for(
        Csm::csmVector<GDCubismEffect*>::iterator i = this->_owner_viewport->_list_cubism_effect.Begin();
        i != this->_owner_viewport->_list_cubism_effect.End();
        i++
    ) {
        switch(efx_call) {
            case EFFECT_CALL_PROLOGUE:  (*i)->_cubism_prologue(this, delta);    break;
            case EFFECT_CALL_PROCESS:   (*i)->_cubism_process(this, delta);     break;
            case EFFECT_CALL_EPILOGUE:  (*i)->_cubism_epilogue(this, delta);    break;
        }
    }
}


Vector2 InternalCubismUserModel::get_size(const Csm::CubismModel *model)
{
    Live2D::Cubism::Core::csmVector2 vct_size;
    Live2D::Cubism::Core::csmVector2 vct_origin;
    Csm::csmFloat32 ppunit;

    Live2D::Cubism::Core::csmReadCanvasInfo(model->GetModel(), &vct_size, &vct_origin, &ppunit);

    return Vector2(vct_size.X, vct_size.Y);
}

Vector2 InternalCubismUserModel::get_origin(const Csm::CubismModel *model)
{
    Live2D::Cubism::Core::csmVector2 vct_size;
    Live2D::Cubism::Core::csmVector2 vct_origin;
    Csm::csmFloat32 ppunit;

    Live2D::Cubism::Core::csmReadCanvasInfo(model->GetModel(), &vct_size, &vct_origin, &ppunit);

    return Vector2(vct_origin.X, vct_origin.Y);
}

float InternalCubismUserModel::get_ppunit(const Csm::CubismModel *model)
{
    Live2D::Cubism::Core::csmVector2 vct_size;
    Live2D::Cubism::Core::csmVector2 vct_origin;
    Csm::csmFloat32 ppunit;

    Live2D::Cubism::Core::csmReadCanvasInfo(model->GetModel(), &vct_size, &vct_origin, &ppunit);

    return ppunit;
}


// ------------------------------------------------------------------ method(s)
