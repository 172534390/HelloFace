#ifndef HIFACE_LINUX_H
#define HIFACE_LINUX_H

#include <hiface/hiface_c_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Load and prepare the SDK's protected built-in CPU models for this Session.
 * Create the Session with enable_feature=1 and enable_landmark=1.
 * Synchronous; serialize with other Session calls. No model files are required.
 * out_report may be NULL; otherwise set its struct_size before calling.
 */
HIFACE_API HiFaceStatus HIFACE_CALL hiface_linux_load_builtin_models_v1(
        HiFaceSessionHandle session, HiFaceModelLoadReportV1* out_report);

/** Load only protected FLIR. No detector, feature or landmarks are needed.
 * Then call hiface_session_evaluate_infrared_liveness_v1. Does not start a flow. */
HIFACE_API HiFaceStatus HIFACE_CALL hiface_linux_load_infrared_model_v1(
        HiFaceSessionHandle session, HiFaceModelLoadReportV1* out_report);

#ifdef __cplusplus
}
#endif
#endif
