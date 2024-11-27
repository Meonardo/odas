
#include "configs.h"

configs *configs_construct(const char *file_config) {
  configs *cfgs;
  unsigned int iSink;

  cfgs = (configs *)malloc(sizeof(configs));

  // +----------------------------------------------------------+
  // | Raw                                                      |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Source                                               |
  // +------------------------------------------------------+

  cfgs->src_hops_mics_config = parameters_src_hops_mics_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_hops_mics_raw_config =
      parameters_msg_hops_mics_raw_config(file_config);

  // +----------------------------------------------------------+
  // | Mapping                                                  |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_mapping_mics_config =
      parameters_mod_mapping_mics_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_hops_mics_map_config =
      parameters_msg_hops_mics_map_config(file_config);

  // +----------------------------------------------------------+
  // | Resample                                                 |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_resample_mics_config =
      parameters_mod_resample_mics_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_hops_mics_rs_config =
      parameters_msg_hops_mics_rs_config(file_config);

  // +----------------------------------------------------------+
  // | STFT                                                     |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_stft_mics_config = parameters_mod_stft_mics_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_spectra_mics_config =
      parameters_msg_spectra_mics_config(file_config);

  // +----------------------------------------------------------+
  // | Noise                                                    |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_noise_mics_config = parameters_mod_noise_mics_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_powers_mics_config = parameters_msg_powers_mics_config(file_config);

  // +----------------------------------------------------------+
  // | SSL                                                      |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_ssl_config = parameters_mod_ssl_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_pots_ssl_config = parameters_msg_pots_ssl_config(file_config);

  // +------------------------------------------------------+
  // | Sinks                                                |
  // +------------------------------------------------------+

  cfgs->snk_pots_ssl_config = parameters_snk_pots_ssl_config(file_config);

  // +----------------------------------------------------------+
  // | Target                                                   |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Injector                                             |
  // +------------------------------------------------------+

  cfgs->inj_targets_sst_config = parameters_inj_targets_sst_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_targets_sst_config = parameters_msg_targets_sst_config(file_config);

  // +----------------------------------------------------------+
  // | SST                                                      |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_sst_config = parameters_mod_sst_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_tracks_sst_config = parameters_msg_tracks_sst_config(file_config);

  // +------------------------------------------------------+
  // | Sinks                                                |
  // +------------------------------------------------------+

  cfgs->snk_tracks_sst_config = parameters_snk_tracks_sst_config(file_config);

  // +----------------------------------------------------------+
  // | SSS                                                      |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_sss_config = parameters_mod_sss_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_spectra_seps_config =
      parameters_msg_spectra_seps_config(file_config);
  cfgs->msg_spectra_pfs_config = parameters_msg_spectra_pfs_config(file_config);

  // +----------------------------------------------------------+
  // | ISTFT                                                    |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_istft_seps_config = parameters_mod_istft_seps_config(file_config);
  cfgs->mod_istft_pfs_config = parameters_mod_istft_pfs_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_hops_seps_config = parameters_msg_hops_seps_config(file_config);
  cfgs->msg_hops_pfs_config = parameters_msg_hops_pfs_config(file_config);

  // +----------------------------------------------------------+
  // | Resample                                                 |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_resample_seps_config =
      parameters_mod_resample_seps_config(file_config);
  cfgs->mod_resample_pfs_config =
      parameters_mod_resample_pfs_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_hops_seps_rs_config =
      parameters_msg_hops_seps_rs_config(file_config);
  cfgs->msg_hops_pfs_rs_config = parameters_msg_hops_pfs_rs_config(file_config);

  // +----------------------------------------------------------+
  // | Volume                                                   |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_volume_seps_config = parameters_mod_volume_seps_config(file_config);
  cfgs->mod_volume_pfs_config = parameters_mod_volume_pfs_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_hops_seps_vol_config =
      parameters_msg_hops_seps_vol_config(file_config);
  cfgs->msg_hops_pfs_vol_config =
      parameters_msg_hops_pfs_vol_config(file_config);

  // +------------------------------------------------------+
  // | Sink                                                 |
  // +------------------------------------------------------+

  cfgs->snk_hops_seps_vol_config =
      parameters_snk_hops_seps_vol_config(file_config);
  cfgs->snk_hops_pfs_vol_config =
      parameters_snk_hops_pfs_vol_config(file_config);

  // +----------------------------------------------------------+
  // | Classify                                                 |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  cfgs->mod_classify_config = parameters_mod_classify_config(file_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  cfgs->msg_categories_config = parameters_msg_categories_config(file_config);

  // +------------------------------------------------------+
  // | Sink                                                 |
  // +------------------------------------------------------+

  cfgs->snk_categories_config = parameters_snk_categories_config(file_config);

  return cfgs;
}

void configs_destroy(configs *cfgs) {
  unsigned int iSink;

  // +----------------------------------------------------------+
  // | Raw                                                      |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Source                                               |
  // +------------------------------------------------------+

  src_hops_cfg_destroy(cfgs->src_hops_mics_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_hops_cfg_destroy(cfgs->msg_hops_mics_raw_config);

  // +----------------------------------------------------------+
  // | Mapping                                                  |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_mapping_cfg_destroy(cfgs->mod_mapping_mics_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_hops_cfg_destroy(cfgs->msg_hops_mics_map_config);

  // +----------------------------------------------------------+
  // | Resample                                                 |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_resample_cfg_destroy(cfgs->mod_resample_mics_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_hops_cfg_destroy(cfgs->msg_hops_mics_rs_config);

  // +----------------------------------------------------------+
  // | STFT                                                     |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_stft_cfg_destroy(cfgs->mod_stft_mics_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_spectra_cfg_destroy(cfgs->msg_spectra_mics_config);

  // +----------------------------------------------------------+
  // | Noise                                                    |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_noise_cfg_destroy(cfgs->mod_noise_mics_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_powers_cfg_destroy(cfgs->msg_powers_mics_config);

  // +----------------------------------------------------------+
  // | SSL                                                      |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_ssl_cfg_destroy(cfgs->mod_ssl_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_pots_cfg_destroy(cfgs->msg_pots_ssl_config);

  // +------------------------------------------------------+
  // | Sink                                                 |
  // +------------------------------------------------------+

  snk_pots_cfg_destroy(cfgs->snk_pots_ssl_config);

  // +----------------------------------------------------------+
  // | Target                                                   |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Injector                                             |
  // +------------------------------------------------------+

  inj_targets_cfg_destroy(cfgs->inj_targets_sst_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_targets_cfg_destroy(cfgs->msg_targets_sst_config);

  // +----------------------------------------------------------+
  // | SST                                                      |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_sst_cfg_destroy(cfgs->mod_sst_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_tracks_cfg_destroy(cfgs->msg_tracks_sst_config);

  // +------------------------------------------------------+
  // | Sink                                                 |
  // +------------------------------------------------------+

  snk_tracks_cfg_destroy(cfgs->snk_tracks_sst_config);

  // +----------------------------------------------------------+
  // | SSS                                                      |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_sss_cfg_destroy(cfgs->mod_sss_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_spectra_cfg_destroy(cfgs->msg_spectra_seps_config);
  msg_spectra_cfg_destroy(cfgs->msg_spectra_pfs_config);

  // +----------------------------------------------------------+
  // | ISTFT                                                    |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_istft_cfg_destroy(cfgs->mod_istft_seps_config);
  mod_istft_cfg_destroy(cfgs->mod_istft_pfs_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_hops_cfg_destroy(cfgs->msg_hops_seps_config);
  msg_hops_cfg_destroy(cfgs->msg_hops_pfs_config);

  // +----------------------------------------------------------+
  // | Resample                                                 |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_resample_cfg_destroy(cfgs->mod_resample_seps_config);
  mod_resample_cfg_destroy(cfgs->mod_resample_pfs_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_hops_cfg_destroy(cfgs->msg_hops_seps_rs_config);
  msg_hops_cfg_destroy(cfgs->msg_hops_pfs_rs_config);

  // +----------------------------------------------------------+
  // | Volume                                                   |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_volume_cfg_destroy(cfgs->mod_volume_seps_config);
  mod_volume_cfg_destroy(cfgs->mod_volume_pfs_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_hops_cfg_destroy(cfgs->msg_hops_seps_vol_config);
  msg_hops_cfg_destroy(cfgs->msg_hops_pfs_vol_config);

  // +------------------------------------------------------+
  // | Sink                                                 |
  // +------------------------------------------------------+

  snk_hops_cfg_destroy(cfgs->snk_hops_seps_vol_config);
  snk_hops_cfg_destroy(cfgs->snk_hops_pfs_vol_config);

  // +----------------------------------------------------------+
  // | Classify                                                 |
  // +----------------------------------------------------------+

  // +------------------------------------------------------+
  // | Module                                               |
  // +------------------------------------------------------+

  mod_classify_cfg_destroy(cfgs->mod_classify_config);

  // +------------------------------------------------------+
  // | Message                                              |
  // +------------------------------------------------------+

  msg_categories_cfg_destroy(cfgs->msg_categories_config);

  // +------------------------------------------------------+
  // | Sink                                                 |
  // +------------------------------------------------------+

  snk_categories_cfg_destroy(cfgs->snk_categories_config);

  free((void *)cfgs);
}
