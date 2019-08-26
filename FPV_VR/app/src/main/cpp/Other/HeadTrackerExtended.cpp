//
// Created by Constantin on 12.12.2017.
//

#include "HeadTrackerExtended.h"
#include "gvr.h"
#include "gvr_types.h"
#include "../SettingsN.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.inl>
#include <android/log.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext.hpp>

#define TAG "HeadTrackerExtended"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)

static gvr::Mat4f MatrixMul(const gvr::Mat4f& matrix1, const gvr::Mat4f& matrix2) {
    gvr::Mat4f result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
            }
        }
    }
    return result;
}

HeadTrackerExtended::HeadTrackerExtended(float interpupilarryDistance) {
    IPD=interpupilarryDistance;
    this->mGroundTrackingMode=S_GHT_MODE;
    this->mEnableAirTracking=S_AHT_MODE==0;
    groundX= S_GHT_X;
    groundY=S_GHT_Y;
    groundZ= S_GHT_Z;

    airYaw=S_AHT_YAW;
    airRoll=S_AHT_ROLL;
    airPitch=S_AHT_PITCH;
}

void HeadTrackerExtended::calculateMatrices(float ratio) {
    worldMatrices.projection=glm::perspective(glm::radians(45.0f), ratio, 0.1f, MAX_Z_DISTANCE+5.0f);
    worldMatrices.projection360=glm::perspective(glm::radians(S_FieldOfView), ratio, 0.1f, MAX_Z_DISTANCE+5.0f);
    worldMatrices.projectionOSD=glm::perspective(glm::radians(45.0f), ratio, 0.1f, MAX_Z_DISTANCE+5.0f);
    worldMatrices.leftEyeView=glm::lookAt(
            glm::vec3(0,0,0),
            glm::vec3(0,0,-MAX_Z_DISTANCE),
            glm::vec3(0,1,0)
    );
    worldMatrices.rightEyeView=glm::lookAt(
            glm::vec3(0,0,0),
            glm::vec3(0,0,-MAX_Z_DISTANCE),
            glm::vec3(0,1,0)
    );
    worldMatrices.monoView=glm::lookAt(
            glm::vec3(0,0,0),
            glm::vec3(0,0,-MAX_Z_DISTANCE),
            glm::vec3(0,1,0)
    );
    glm::translate(worldMatrices.leftEyeView,glm::vec3(-IPD/2.0f,0,0));
    glm::translate(worldMatrices.rightEyeView,glm::vec3(IPD/2.0f,0,0));
}

void HeadTrackerExtended::calculateNewHeadPose(gvr::GvrApi *gvr_api, const int predictMS) {
    if(mGroundTrackingMode==MODE_NONE){
        LOGV("cannot calculate new head pose. False tracking mode.");
        return;
    }
    gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
    target_time.monotonic_system_time_nanos+=predictMS*1000*1000;
    gvr::Mat4f tmpM;
    tmpM=gvr_api->GetHeadSpaceFromStartSpaceRotation(target_time);
    //we only apply the neck model when we calculate the view M for 1PP
    if(mGroundTrackingMode==MODE_1PP){
        gvr_api->ApplyNeckModel(tmpM,1);
    }
    glm::mat4x4 headView=glm::make_mat4(reinterpret_cast<const float*>(&tmpM.m));
    headView=glm::transpose(headView);
    //We can lock head tracking on specific axises. Therefore, we first calculate the inverse quaternion from the headView (rotation) matrix.
    //when we multiply the inverse and the headView at the end they cancel each other out.
    //Therefore, for each axis we want to use we set the rotation on the inverse to zero.
    if(!groundX || !groundY || !groundZ){
        glm::quat quatRotInv = glm::inverse(glm::quat_cast(headView));
        glm::vec3 euler=glm::eulerAngles(quatRotInv);
        if(groundX){
            euler.x=0;
        }
        if(groundY){
            euler.y=0;
        }
        if(groundZ){
            euler.z=0;
        }
        glm::quat quat=glm::quat(euler);
        glm::mat4x4 RotationMatrix = glm::toMat4(quat);
        headView=headView*RotationMatrix;
    }
    worldMatrices.leftEyeViewTracked=glm::translate(headView,glm::vec3(-IPD/2.0f,0,0));
    worldMatrices.rightEyeViewTracked=glm::translate(headView,glm::vec3(IPD/2.0f,0,0));
    worldMatrices.monoViewTracked=glm::translate(headView,glm::vec3(0,0,0.0));
}

gvr::Mat4f MatrixMul(const glm::mat4x4 &m1, const gvr::Mat4f &m2) {
   glm::mat4x4 tm2=glm::make_mat4(reinterpret_cast<const float*>(&m2.m));
    tm2 = m1 * tm2;
    gvr::Mat4f ret;
    memcpy(reinterpret_cast<float*>(&ret.m), reinterpret_cast<float*>(&tm2[0]), 16 * sizeof(float));
    return ret;
}

void HeadTrackerExtended::calculateNewHeadPose360(gvr::GvrApi *gvr_api, const int predictMS) {
    gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
    target_time.monotonic_system_time_nanos+=predictMS*1000*1000;
    gvr::Mat4f tmpM = gvr_api->GetHeadSpaceFromStartSpaceTransform(target_time);
    tmpM = MatrixMul(worldMatrices.monoForward360, tmpM);
    gvr_api->ApplyNeckModel(tmpM,1);
    glm::mat4x4 headView=glm::make_mat4(reinterpret_cast<const float*>(&tmpM.m));
    headView=glm::transpose(headView);
    worldMatrices.monoViewTracked=glm::translate(headView,glm::vec3(0,0,0.0));
}

WorldMatrices* HeadTrackerExtended::getWorldMatrices() {
    return &worldMatrices;
}

glm::mat4x4 startToHeadRotation(gvr::GvrApi *gvr_api) {
    gvr::Mat4f mat = gvr_api->GetHeadSpaceFromStartSpaceTransform(gvr::GvrApi::GetTimePointNow());
    glm::mat4x4 tmat=glm::make_mat4(reinterpret_cast<const float*>(&mat.m));
    return glm::toMat4(glm::quat_cast(tmat));
}

void HeadTrackerExtended::setHomeOrientation(gvr::GvrApi *gvr_api) {
    // Get the current start->head transformation
    worldMatrices.monoForward360=worldMatrices.monoForward360*startToHeadRotation(gvr_api);
    // Reset yaw back to start
    gvr_api->RecenterTracking();
}

void HeadTrackerExtended::goToHomeOrientation(gvr::GvrApi *gvr_api) {
    gvr_api->RecenterTracking();
}
