#ifndef MONOCLE_INTERNAL_H_
#define MONOCLE_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* This isn't really exciting, it just defines some symbols used cross-subsystem but not exported. */

void mncl_uninit_raw_system(void);
void mncl_renormalize_all_spritesheets(void);

#ifdef __cplusplus
}
#endif

#endif
