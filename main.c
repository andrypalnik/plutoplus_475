#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include "linux_spi.h"
#include "linux_gpio.h"
#include "no_os_delay.h" 
#include "no_os_spi.h"
#include "no_os_error.h"
#include "no_os_axi_io.h"
#include "no_os_alloc.h"
//#include "no-OS/drivers/axi_core/axi_dmac/axi_dmac.h"

#include "ad9361.h"
#include "ad9361_api.h"

AD9361_InitParam default_init_param = {
	/* Device selection */
	ID_AD9361,	// dev_sel
	/* Reference Clock */
	40000000UL,	//reference_clk_rate
	/* Base Configuration */
	1,		//two_rx_two_tx_mode_enable *** adi,2rx-2tx-mode-enable
	0,		//one_rx_one_tx_mode_use_rx_num *** adi,1rx-1tx-mode-use-rx-num
	0,		//one_rx_one_tx_mode_use_tx_num *** adi,1rx-1tx-mode-use-tx-num
	1,		//frequency_division_duplex_mode_enable *** adi,frequency-division-duplex-mode-enable
	0,		//frequency_division_duplex_independent_mode_enable *** adi,frequency-division-duplex-independent-mode-enable
	0,		//tdd_use_dual_synth_mode_enable *** adi,tdd-use-dual-synth-mode-enable
	0,		//tdd_skip_vco_cal_enable *** adi,tdd-skip-vco-cal-enable
	0,		//tx_fastlock_delay_ns *** adi,tx-fastlock-delay-ns
	0,		//rx_fastlock_delay_ns *** adi,rx-fastlock-delay-ns
	0,		//rx_fastlock_pincontrol_enable *** adi,rx-fastlock-pincontrol-enable
	0,		//tx_fastlock_pincontrol_enable *** adi,tx-fastlock-pincontrol-enable
	0,		//external_rx_lo_enable *** adi,external-rx-lo-enable
	0,		//external_tx_lo_enable *** adi,external-tx-lo-enable
	5,		//dc_offset_tracking_update_event_mask *** adi,dc-offset-tracking-update-event-mask
	6,		//dc_offset_attenuation_high_range *** adi,dc-offset-attenuation-high-range
	5,		//dc_offset_attenuation_low_range *** adi,dc-offset-attenuation-low-range
	0x28,	//dc_offset_count_high_range *** adi,dc-offset-count-high-range
	0x32,	//dc_offset_count_low_range *** adi,dc-offset-count-low-range
	0,		//split_gain_table_mode_enable *** adi,split-gain-table-mode-enable
	MAX_SYNTH_FREF,	//trx_synthesizer_target_fref_overwrite_hz *** adi,trx-synthesizer-target-fref-overwrite-hz
	0,		// qec_tracking_slow_mode_enable *** adi,qec-tracking-slow-mode-enable
	/* ENSM Control */
	0,		//ensm_enable_pin_pulse_mode_enable *** adi,ensm-enable-pin-pulse-mode-enable
	0,		//ensm_enable_txnrx_control_enable *** adi,ensm-enable-txnrx-control-enable
	/* LO Control */
	1080000000UL,	//rx_synthesizer_frequency_hz *** adi,rx-synthesizer-frequency-hz
	1080000000UL,	//tx_synthesizer_frequency_hz *** adi,tx-synthesizer-frequency-hz
	0,				//tx_lo_powerdown_managed_enable *** adi,tx-lo-powerdown-managed-enable
	/* Rate & BW Control */
	{983040000, 245760000, 122880000, 61440000, 30720000, 30720000},// rx_path_clock_frequencies[6] *** adi,rx-path-clock-frequencies
	{983040000, 122880000, 122880000, 61440000, 30720000, 30720000},// tx_path_clock_frequencies[6] *** adi,tx-path-clock-frequencies
	5000000,//rf_rx_bandwidth_hz *** adi,rf-rx-bandwidth-hz
	5000000,//rf_tx_bandwidth_hz *** adi,rf-tx-bandwidth-hz
	/* RF Port Control */
	0,		//rx_rf_port_input_select *** adi,rx-rf-port-input-select
	0,		//tx_rf_port_input_select *** adi,tx-rf-port-input-select
	/* TX Attenuation Control */
	10000,	//tx_attenuation_mdB *** adi,tx-attenuation-mdB
	0,		//update_tx_gain_in_alert_enable *** adi,update-tx-gain-in-alert-enable
	/* Reference Clock Control */
	0,		//xo_disable_use_ext_refclk_enable *** adi,xo-disable-use-ext-refclk-enable
	{8, 5920},	//dcxo_coarse_and_fine_tune[2] *** adi,dcxo-coarse-and-fine-tune
	CLKOUT_DISABLE,	//clk_output_mode_select *** adi,clk-output-mode-select
	/* Gain Control */
	2,		//gc_rx1_mode *** adi,gc-rx1-mode
	2,		//gc_rx2_mode *** adi,gc-rx2-mode
	58,		//gc_adc_large_overload_thresh *** adi,gc-adc-large-overload-thresh
	4,		//gc_adc_ovr_sample_size *** adi,gc-adc-ovr-sample-size
	47,		//gc_adc_small_overload_thresh *** adi,gc-adc-small-overload-thresh
	8192,	//gc_dec_pow_measurement_duration *** adi,gc-dec-pow-measurement-duration
	0,		//gc_dig_gain_enable *** adi,gc-dig-gain-enable
	800,	//gc_lmt_overload_high_thresh *** adi,gc-lmt-overload-high-thresh
	704,	//gc_lmt_overload_low_thresh *** adi,gc-lmt-overload-low-thresh
	24,		//gc_low_power_thresh *** adi,gc-low-power-thresh
	15,		//gc_max_dig_gain *** adi,gc-max-dig-gain
	0,		//gc_use_rx_fir_out_for_dec_pwr_meas_enable *** adi,gc-use-rx-fir-out-for-dec-pwr-meas-enable
	/* Gain MGC Control */
	2,		//mgc_dec_gain_step *** adi,mgc-dec-gain-step
	2,		//mgc_inc_gain_step *** adi,mgc-inc-gain-step
	0,		//mgc_rx1_ctrl_inp_enable *** adi,mgc-rx1-ctrl-inp-enable
	0,		//mgc_rx2_ctrl_inp_enable *** adi,mgc-rx2-ctrl-inp-enable
	0,		//mgc_split_table_ctrl_inp_gain_mode *** adi,mgc-split-table-ctrl-inp-gain-mode
	/* Gain AGC Control */
	10,		//agc_adc_large_overload_exceed_counter *** adi,agc-adc-large-overload-exceed-counter
	2,		//agc_adc_large_overload_inc_steps *** adi,agc-adc-large-overload-inc-steps
	0,		//agc_adc_lmt_small_overload_prevent_gain_inc_enable *** adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable
	10,		//agc_adc_small_overload_exceed_counter *** adi,agc-adc-small-overload-exceed-counter
	4,		//agc_dig_gain_step_size *** adi,agc-dig-gain-step-size
	3,		//agc_dig_saturation_exceed_counter *** adi,agc-dig-saturation-exceed-counter
	1000,	// agc_gain_update_interval_us *** adi,agc-gain-update-interval-us
	0,		//agc_immed_gain_change_if_large_adc_overload_enable *** adi,agc-immed-gain-change-if-large-adc-overload-enable
	0,		//agc_immed_gain_change_if_large_lmt_overload_enable *** adi,agc-immed-gain-change-if-large-lmt-overload-enable
	10,		//agc_inner_thresh_high *** adi,agc-inner-thresh-high
	1,		//agc_inner_thresh_high_dec_steps *** adi,agc-inner-thresh-high-dec-steps
	12,		//agc_inner_thresh_low *** adi,agc-inner-thresh-low
	1,		//agc_inner_thresh_low_inc_steps *** adi,agc-inner-thresh-low-inc-steps
	10,		//agc_lmt_overload_large_exceed_counter *** adi,agc-lmt-overload-large-exceed-counter
	2,		//agc_lmt_overload_large_inc_steps *** adi,agc-lmt-overload-large-inc-steps
	10,		//agc_lmt_overload_small_exceed_counter *** adi,agc-lmt-overload-small-exceed-counter
	5,		//agc_outer_thresh_high *** adi,agc-outer-thresh-high
	2,		//agc_outer_thresh_high_dec_steps *** adi,agc-outer-thresh-high-dec-steps
	18,		//agc_outer_thresh_low *** adi,agc-outer-thresh-low
	2,		//agc_outer_thresh_low_inc_steps *** adi,agc-outer-thresh-low-inc-steps
	1,		//agc_attack_delay_extra_margin_us; *** adi,agc-attack-delay-extra-margin-us
	0,		//agc_sync_for_gain_counter_enable *** adi,agc-sync-for-gain-counter-enable
	/* Fast AGC */
	64,		//fagc_dec_pow_measuremnt_duration ***  adi,fagc-dec-pow-measurement-duration
	260,	//fagc_state_wait_time_ns ***  adi,fagc-state-wait-time-ns
	/* Fast AGC - Low Power */
	0,		//fagc_allow_agc_gain_increase ***  adi,fagc-allow-agc-gain-increase-enable
	5,		//fagc_lp_thresh_increment_time ***  adi,fagc-lp-thresh-increment-time
	1,		//fagc_lp_thresh_increment_steps ***  adi,fagc-lp-thresh-increment-steps
	/* Fast AGC - Lock Level (Lock Level is set via slow AGC inner high threshold) */
	1,		//fagc_lock_level_lmt_gain_increase_en ***  adi,fagc-lock-level-lmt-gain-increase-enable
	5,		//fagc_lock_level_gain_increase_upper_limit ***  adi,fagc-lock-level-gain-increase-upper-limit
	/* Fast AGC - Peak Detectors and Final Settling */
	1,		//fagc_lpf_final_settling_steps ***  adi,fagc-lpf-final-settling-steps
	1,		//fagc_lmt_final_settling_steps ***  adi,fagc-lmt-final-settling-steps
	3,		//fagc_final_overrange_count ***  adi,fagc-final-overrange-count
	/* Fast AGC - Final Power Test */
	0,		//fagc_gain_increase_after_gain_lock_en ***  adi,fagc-gain-increase-after-gain-lock-enable
	/* Fast AGC - Unlocking the Gain */
	0,		//fagc_gain_index_type_after_exit_rx_mode ***  adi,fagc-gain-index-type-after-exit-rx-mode
	1,		//fagc_use_last_lock_level_for_set_gain_en ***  adi,fagc-use-last-lock-level-for-set-gain-enable
	1,		//fagc_rst_gla_stronger_sig_thresh_exceeded_en ***  adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable
	5,		//fagc_optimized_gain_offset ***  adi,fagc-optimized-gain-offset
	10,		//fagc_rst_gla_stronger_sig_thresh_above_ll ***  adi,fagc-rst-gla-stronger-sig-thresh-above-ll
	1,		//fagc_rst_gla_engergy_lost_sig_thresh_exceeded_en ***  adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable
	1,		//fagc_rst_gla_engergy_lost_goto_optim_gain_en ***  adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable
	10,		//fagc_rst_gla_engergy_lost_sig_thresh_below_ll ***  adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll
	8,		//fagc_energy_lost_stronger_sig_gain_lock_exit_cnt ***  adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt
	1,		//fagc_rst_gla_large_adc_overload_en ***  adi,fagc-rst-gla-large-adc-overload-enable
	1,		//fagc_rst_gla_large_lmt_overload_en ***  adi,fagc-rst-gla-large-lmt-overload-enable
	0,		//fagc_rst_gla_en_agc_pulled_high_en ***  adi,fagc-rst-gla-en-agc-pulled-high-enable
	0,		//fagc_rst_gla_if_en_agc_pulled_high_mode ***  adi,fagc-rst-gla-if-en-agc-pulled-high-mode
	64,		//fagc_power_measurement_duration_in_state5 ***  adi,fagc-power-measurement-duration-in-state5
	2,		//fagc_large_overload_inc_steps *** adi,fagc-adc-large-overload-inc-steps
	/* RSSI Control */
	1,		//rssi_delay *** adi,rssi-delay
	1000,	//rssi_duration *** adi,rssi-duration
	3,		//rssi_restart_mode *** adi,rssi-restart-mode
	0,		//rssi_unit_is_rx_samples_enable *** adi,rssi-unit-is-rx-samples-enable
	1,		//rssi_wait *** adi,rssi-wait
	/* Aux ADC Control */
	256,	//aux_adc_decimation *** adi,aux-adc-decimation
	40000000UL,	//aux_adc_rate *** adi,aux-adc-rate
	/* AuxDAC Control */
	1,		//aux_dac_manual_mode_enable ***  adi,aux-dac-manual-mode-enable
	0,		//aux_dac1_default_value_mV ***  adi,aux-dac1-default-value-mV
	0,		//aux_dac1_active_in_rx_enable ***  adi,aux-dac1-active-in-rx-enable
	0,		//aux_dac1_active_in_tx_enable ***  adi,aux-dac1-active-in-tx-enable
	0,		//aux_dac1_active_in_alert_enable ***  adi,aux-dac1-active-in-alert-enable
	0,		//aux_dac1_rx_delay_us ***  adi,aux-dac1-rx-delay-us
	0,		//aux_dac1_tx_delay_us ***  adi,aux-dac1-tx-delay-us
	0,		//aux_dac2_default_value_mV ***  adi,aux-dac2-default-value-mV
	0,		//aux_dac2_active_in_rx_enable ***  adi,aux-dac2-active-in-rx-enable
	0,		//aux_dac2_active_in_tx_enable ***  adi,aux-dac2-active-in-tx-enable
	0,		//aux_dac2_active_in_alert_enable ***  adi,aux-dac2-active-in-alert-enable
	0,		//aux_dac2_rx_delay_us ***  adi,aux-dac2-rx-delay-us
	0,		//aux_dac2_tx_delay_us ***  adi,aux-dac2-tx-delay-us
	/* Temperature Sensor Control */
	256,	//temp_sense_decimation *** adi,temp-sense-decimation
	1000,	//temp_sense_measurement_interval_ms *** adi,temp-sense-measurement-interval-ms
	0xCE,	//temp_sense_offset_signed *** adi,temp-sense-offset-signed
	1,		//temp_sense_periodic_measurement_enable *** adi,temp-sense-periodic-measurement-enable
	/* Control Out Setup */
	0xFF,	//ctrl_outs_enable_mask *** adi,ctrl-outs-enable-mask
	0,		//ctrl_outs_index *** adi,ctrl-outs-index
	/* External LNA Control */
	0,		//elna_settling_delay_ns *** adi,elna-settling-delay-ns
	0,		//elna_gain_mdB *** adi,elna-gain-mdB
	0,		//elna_bypass_loss_mdB *** adi,elna-bypass-loss-mdB
	0,		//elna_rx1_gpo0_control_enable *** adi,elna-rx1-gpo0-control-enable
	0,		//elna_rx2_gpo1_control_enable *** adi,elna-rx2-gpo1-control-enable
	0,		//elna_gaintable_all_index_enable *** adi,elna-gaintable-all-index-enable
	/* Digital Interface Control */
	0,		//digital_interface_tune_skip_mode *** adi,digital-interface-tune-skip-mode
	0,		//digital_interface_tune_fir_disable *** adi,digital-interface-tune-fir-disable
	1,		//pp_tx_swap_enable *** adi,pp-tx-swap-enable
	1,		//pp_rx_swap_enable *** adi,pp-rx-swap-enable
	0,		//tx_channel_swap_enable *** adi,tx-channel-swap-enable
	0,		//rx_channel_swap_enable *** adi,rx-channel-swap-enable
	1,		//rx_frame_pulse_mode_enable *** adi,rx-frame-pulse-mode-enable
	0,		//two_t_two_r_timing_enable *** adi,2t2r-timing-enable
	0,		//invert_data_bus_enable *** adi,invert-data-bus-enable
	0,		//invert_data_clk_enable *** adi,invert-data-clk-enable
	0,		//fdd_alt_word_order_enable *** adi,fdd-alt-word-order-enable
	0,		//invert_rx_frame_enable *** adi,invert-rx-frame-enable
	0,		//fdd_rx_rate_2tx_enable *** adi,fdd-rx-rate-2tx-enable
	1,		//swap_ports_enable *** adi,swap-ports-enable
	0,		//single_data_rate_enable *** adi,single-data-rate-enable
	0,		//lvds_mode_enable *** adi,lvds-mode-enable
	0,		//half_duplex_mode_enable *** adi,half-duplex-mode-enable
	0,		//single_port_mode_enable *** adi,single-port-mode-enable
	1,		//full_port_enable *** adi,full-port-enable
	0,		//full_duplex_swap_bits_enable *** adi,full-duplex-swap-bits-enable
	0,		//delay_rx_data *** adi,delay-rx-data
	0,		//rx_data_clock_delay *** adi,rx-data-clock-delay
	4,		//rx_data_delay *** adi,rx-data-delay
	7,		//tx_fb_clock_delay *** adi,tx-fb-clock-delay
	0,		//tx_data_delay *** adi,tx-data-delay
#ifdef ALTERA_PLATFORM
	300,	//lvds_bias_mV *** adi,lvds-bias-mV
#else
	150,	//lvds_bias_mV *** adi,lvds-bias-mV
#endif
	0,		//lvds_rx_onchip_termination_enable *** adi,lvds-rx-onchip-termination-enable
	0,		//rx1rx2_phase_inversion_en *** adi,rx1-rx2-phase-inversion-enable
	0xFF,	//lvds_invert1_control *** adi,lvds-invert1-control
	0x0F,	//lvds_invert2_control *** adi,lvds-invert2-control
	/* GPO Control */
	0,		//gpo_manual_mode_enable *** adi,gpo-manual-mode-enable
	0,		//gpo_manual_mode_enable_mask *** adi,gpo-manual-mode-enable-mask
	0,		//gpo0_inactive_state_high_enable *** adi,gpo0-inactive-state-high-enable
	0,		//gpo1_inactive_state_high_enable *** adi,gpo1-inactive-state-high-enable
	0,		//gpo2_inactive_state_high_enable *** adi,gpo2-inactive-state-high-enable
	0,		//gpo3_inactive_state_high_enable *** adi,gpo3-inactive-state-high-enable
	0,		//gpo0_slave_rx_enable *** adi,gpo0-slave-rx-enable
	0,		//gpo0_slave_tx_enable *** adi,gpo0-slave-tx-enable
	0,		//gpo1_slave_rx_enable *** adi,gpo1-slave-rx-enable
	0,		//gpo1_slave_tx_enable *** adi,gpo1-slave-tx-enable
	0,		//gpo2_slave_rx_enable *** adi,gpo2-slave-rx-enable
	0,		//gpo2_slave_tx_enable *** adi,gpo2-slave-tx-enable
	0,		//gpo3_slave_rx_enable *** adi,gpo3-slave-rx-enable
	0,		//gpo3_slave_tx_enable *** adi,gpo3-slave-tx-enable
	0,		//gpo0_rx_delay_us *** adi,gpo0-rx-delay-us
	0,		//gpo0_tx_delay_us *** adi,gpo0-tx-delay-us
	0,		//gpo1_rx_delay_us *** adi,gpo1-rx-delay-us
	0,		//gpo1_tx_delay_us *** adi,gpo1-tx-delay-us
	0,		//gpo2_rx_delay_us *** adi,gpo2-rx-delay-us
	0,		//gpo2_tx_delay_us *** adi,gpo2-tx-delay-us
	0,		//gpo3_rx_delay_us *** adi,gpo3-rx-delay-us
	0,		//gpo3_tx_delay_us *** adi,gpo3-tx-delay-us
	/* Tx Monitor Control */
	37000,	//low_high_gain_threshold_mdB *** adi,txmon-low-high-thresh
	0,		//low_gain_dB *** adi,txmon-low-gain
	24,		//high_gain_dB *** adi,txmon-high-gain
	0,		//tx_mon_track_en *** adi,txmon-dc-tracking-enable
	0,		//one_shot_mode_en *** adi,txmon-one-shot-mode-enable
	511,	//tx_mon_delay *** adi,txmon-delay
	8192,	//tx_mon_duration *** adi,txmon-duration
	2,		//tx1_mon_front_end_gain *** adi,txmon-1-front-end-gain
	2,		//tx2_mon_front_end_gain *** adi,txmon-2-front-end-gain
	48,		//tx1_mon_lo_cm *** adi,txmon-1-lo-cm
	48,		//tx2_mon_lo_cm *** adi,txmon-2-lo-cm
	/* GPIO definitions */
	{
		.number = -1,
		.platform_ops = &linux_gpio_ops,
		.extra = "/dev/gpiochip0"
	},		//gpio_resetb *** reset-gpios
	/* MCS Sync */
	{
		.number = -1,
		.platform_ops = &linux_gpio_ops,
		.extra = "/dev/gpiochip0"
	},		//gpio_sync *** sync-gpios

	{
		.number = -1,
		.platform_ops = &linux_gpio_ops,
		.extra = "/dev/gpiochip0"
	},		//gpio_cal_sw1 *** cal-sw1-gpios

	{
		.number = -1,
		.platform_ops = &linux_gpio_ops,
		.extra = "/dev/gpiochip0"
	},		//gpio_cal_sw2 *** cal-sw2-gpios

	{
		.device_id = 1,
		.mode = NO_OS_SPI_MODE_1,
		.chip_select = 0,
		.platform_ops = &linux_spi_ops,
		.extra = "/dev/spidev1.0"
	},

	/* External LO clocks */
	NULL,	//(*ad9361_rfpll_ext_recalc_rate)()
	NULL,	//(*ad9361_rfpll_ext_round_rate)()
	NULL,	//(*ad9361_rfpll_ext_set_rate)()
};

AD9361_RXFIRConfig rx_fir_config = {	// BPF PASSBAND 3/20 fs to 1/4 fs
	3, // rx
	0, // rx_gain
	1, // rx_dec
	{
		-4, -6, -37, 35, 186, 86, -284, -315,
			107, 219, -4, 271, 558, -307, -1182, -356,
			658, 157, 207, 1648, 790, -2525, -2553, 748,
			865, -476, 3737, 6560, -3583, -14731, -5278, 14819,
			14819, -5278, -14731, -3583, 6560, 3737, -476, 865,
			748, -2553, -2525, 790, 1648, 207, 157, 658,
			-356, -1182, -307, 558, 271, -4, 219, 107,
			-315, -284, 86, 186, 35, -37, -6, -4,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		}, // rx_coef[128]
	64, // rx_coef_size
	{0, 0, 0, 0, 0, 0}, //rx_path_clks[6]
	0 // rx_bandwidth
};

AD9361_TXFIRConfig tx_fir_config = {	// BPF PASSBAND 3/20 fs to 1/4 fs
	3, // tx
	-6, // tx_gain
	1, // tx_int
	{
		-4, -6, -37, 35, 186, 86, -284, -315,
			107, 219, -4, 271, 558, -307, -1182, -356,
			658, 157, 207, 1648, 790, -2525, -2553, 748,
			865, -476, 3737, 6560, -3583, -14731, -5278, 14819,
			14819, -5278, -14731, -3583, 6560, 3737, -476, 865,
			748, -2553, -2525, 790, 1648, 207, 157, 658,
			-356, -1182, -307, 558, 271, -4, 219, 107,
			-315, -284, 86, 186, 35, -37, -6, -4,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		}, // tx_coef[128]
	64, // tx_coef_size
	{0, 0, 0, 0, 0, 0}, // tx_path_clks[6]
	0 // tx_bandwidth
};

struct no_os_spi_init_param spi_param = {
	.device_id = 1,
	.max_speed_hz = 10000000,
	.mode = NO_OS_SPI_MODE_0,
	.chip_select = 0,
	.platform_ops = &linux_spi_ops,
	.extra = "/dev/spidev1.0"
};

// DDR read -> file
#define DMA_BASEADDR      0x40400000
#define DDR_DEST_ADDR     0x1F400000
#define TRANSFER_SIZE     (4 * 1024 * 1024) // 4 MB

// DMA S2MM register offsets
#define DMA_S2MM_CONTROL  0x30
#define DMA_S2MM_STATUS   0x34
#define DMA_S2MM_DEST     0x48
#define DMA_S2MM_LENGTH   0x58

// #define DMA_BASEADDR 0x40400000 // Заміни на свою!
// #define DMA_TRANSFER_LEN  (4*1024*1024) // 4 МБ, скільки семплів хочеш

// int dma_read_and_save(struct axi_dmac *dma, uint32_t *buf, uint32_t len)
// {
//     int ret;
//     ret = axi_dmac_transfer(dma, (uint32_t)buf, len, DMA_DEV_TO_MEM);
//     if (ret) {
//         printf("DMA transfer failed: %d\n", ret);
//         return ret;
//     }
//     // Дочекайся завершення
//     ret = axi_dmac_wait_for_completion(dma, 100000);
//     if (ret) {
//         printf("DMA wait for completion failed: %d\n", ret);
//         return ret;
//     }
//     return 0;
// }

// struct axi_dmac_init dma_init = {
// 	.name = "rx_dma",
// 	.base = DMA_BASEADDR,
// 	.irq_option = IRQ_DISABLED,
// };

//#include "no-OS/drivers/axi_core/axi_dmac/axi_dmac.h"

// ... (інші інклуди, як у тебе в коді)
#if 0
#define DMAC_BASEADDR   0x40400000   // Твоя адреса DMA (з Address Editor)
#define MEM_BUF_ADDR    0x1F400000  // Наприклад, адреса у DDR, вирішуй сам (НЕ конфліктуй з ядром!)

// Розмір буфера
#define NUM_SAMPLES     1024*1024  // 1 мегасемпл, змінюй під себе
#define TIMEOUT_MS      5000       // 5 секунд тайм-аут

void dma_read_and_save(void);

void dma_read_and_save() {
    int ret;
    struct axi_dmac *dmac;
    struct axi_dmac_init dma_init = {
        .name = "rx_dma",
        .base = DMAC_BASEADDR,
        .irq_option = IRQ_DISABLED,
    };
    // 1. Ініціалізуємо DMA
    ret = axi_dmac_init(&dmac, &dma_init);
    if (ret < 0) {
        printf("DMA init error\n");
        return;
    }

    // 2. Підготовка буфера
    void *sample_buffer = malloc(NUM_SAMPLES * sizeof(uint32_t)); // або uint16_t, якщо 16 бітів
    if (!sample_buffer) {
        printf("Buffer alloc error\n");
        axi_dmac_remove(dmac);
        return;
    }

    // 3. Запуск DMA на читання (Device-to-Mem)
    struct axi_dma_transfer transfer = {
        .size = NUM_SAMPLES * sizeof(uint32_t),
        .cyclic = NO,
        .src_addr = 0,             // джерело - FIFO (DMAC сам знає, це RX DMA)
        .dest_addr = MEM_BUF_ADDR, // Куди писати (має бути в DDR, переконайся, що ця адреса не зайнята)
    };

    ret = axi_dmac_transfer_start(dmac, &transfer);
    if (ret < 0) {
        printf("DMA transfer start error\n");
        free(sample_buffer);
        axi_dmac_remove(dmac);
        return;
    }

    // 4. Чекаємо завершення DMA
    ret = axi_dmac_transfer_wait_completion(dmac, TIMEOUT_MS);
    if (ret < 0) {
        printf("DMA transfer timeout/error\n");
        free(sample_buffer);
        axi_dmac_remove(dmac);
        return;
    }
    printf("DMA Transfer complete\n");

    // 5. Копіюємо з DDR у RAM (user space)
    // mmap() або /dev/mem — якщо працюєш з baremetal/no-OS, у Petalinux mmap() на /dev/mem:
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        printf("Open /dev/mem failed\n");
        free(sample_buffer);
        axi_dmac_remove(dmac);
        return;
    }
    void *mapped_mem = mmap(NULL, NUM_SAMPLES * sizeof(uint32_t), PROT_READ, MAP_SHARED, mem_fd, MEM_BUF_ADDR);
    if (mapped_mem == MAP_FAILED) {
        printf("mmap failed\n");
        close(mem_fd);
        free(sample_buffer);
        axi_dmac_remove(dmac);
        return;
    }

    memcpy(sample_buffer, mapped_mem, NUM_SAMPLES * sizeof(uint32_t));
    munmap(mapped_mem, NUM_SAMPLES * sizeof(uint32_t));
    close(mem_fd);

    // 6. Записуємо у файл на SD
    FILE *f = fopen("/media/card/data.bin", "wb");
    if (!f) {
        printf("Can't open file on SD\n");
    } else {
        fwrite(sample_buffer, 1, NUM_SAMPLES * sizeof(uint32_t), f);
        fclose(f);
        printf("Saved %d bytes to SD\n", NUM_SAMPLES * sizeof(uint32_t));
    }

    // 7. Clean up
    free(sample_buffer);
    axi_dmac_remove(dmac);
}
#endif

#if 0
#include <sys/ioctl.h>

#define DMA_PROXY_BUF_SIZE (4 * 1024 * 1024)  // розмір має відповідати тому, що виділяється драйвером
#define START_XFER   _IOW('q', 1, int)
#define FINISH_XFER  _IOW('q', 2, int)

void dma_proxy_read_and_save(void);

void dma_proxy_read_and_save() {
    int fd_rx = open("/dev/dma_proxy_rx", O_RDWR);
    if (fd_rx < 0) {
        perror("Open /dev/dma_proxy_rx failed");
        return;
    }

    // mmap() DMA буфера
    void *rx_buffer = mmap(NULL, DMA_PROXY_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_rx, 0);
    if (rx_buffer == MAP_FAILED) {
        perror("mmap failed");
        close(fd_rx);
        return;
    }

    int buf_idx = 0; // Зазвичай 0, бо в драйвері BUFFER_COUNT=1

    // Початок DMA
    if (ioctl(fd_rx, START_XFER, &buf_idx) < 0) {
        perror("START_XFER ioctl failed");
        goto cleanup;
    }

	sleep(2);

    // Чекати завершення
    if (ioctl(fd_rx, FINISH_XFER, &buf_idx) < 0) {
        perror("FINISH_XFER ioctl failed");
        goto cleanup;
    }

    // Зберегти у файл
    FILE *f = fopen("/media/card/dma_proxy_data.bin", "wb");
    if (!f) {
        perror("Can't open file");
        goto cleanup;
    }
    fwrite(rx_buffer, 1, DMA_PROXY_BUF_SIZE, f);
    fclose(f);
    printf("DMA proxy saved %d bytes to SD\n", DMA_PROXY_BUF_SIZE);

cleanup:
    munmap(rx_buffer, DMA_PROXY_BUF_SIZE);
    close(fd_rx);
}
#endif

#if 1

void dma_ddr_read_and_save(void);

void dma_ddr_read_and_save() {
    int fd = -1;
    volatile uint32_t *dma_regs = NULL;
    void *ddr_buffer = NULL;

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return;
    }

    dma_regs = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, DMA_BASEADDR);
    if (dma_regs == MAP_FAILED) {
        perror("mmap dma_regs");
        close(fd);
        return;
    }

    ddr_buffer = mmap(NULL, TRANSFER_SIZE, PROT_READ, MAP_SHARED, fd, DDR_DEST_ADDR);
    if (ddr_buffer == MAP_FAILED) {
        perror("mmap ddr_buffer");
        munmap((void*)dma_regs, 4096);
        close(fd);
        return;
    }

    // Reset DMA (bit 2), потім enable (bit 0)
    dma_regs[DMA_S2MM_CONTROL / 4] = 0x4;
    usleep(1000);
    dma_regs[DMA_S2MM_CONTROL / 4] = 0x1;

    // Clear status (записом тих самих бітів що хочемо скинути)
    dma_regs[DMA_S2MM_STATUS / 4] = 0xFFFFFFFF;

    printf("Before DEST write: 0x%08X\n", dma_regs[DMA_S2MM_DEST / 4]);
    dma_regs[DMA_S2MM_DEST / 4] = DDR_DEST_ADDR;
    printf("After DEST write: 0x%08X\n", dma_regs[DMA_S2MM_DEST / 4]);

    dma_regs[DMA_S2MM_LENGTH / 4] = TRANSFER_SIZE;
    printf("After LEN write: 0x%08X\n", dma_regs[DMA_S2MM_LENGTH / 4]);

    printf("Waiting for DMA transfer to complete...\n");

    // Очікуємо bit 12 (IOC_Irq)
    int timeout = 100000; // ~100 сек (в мс)
    while (!(dma_regs[DMA_S2MM_STATUS / 4] & (1 << 12))) {
        usleep(1000);
        timeout--;
        if (timeout <= 0) {
            fprintf(stderr, "DMA timeout! Status: 0x%08X\n", dma_regs[DMA_S2MM_STATUS / 4]);
            goto cleanup;
        }
    }

    printf("DMA transfer complete! Status: 0x%08X\n", dma_regs[DMA_S2MM_STATUS / 4]);

    FILE *f = fopen("/media/card/dma_dump.bin", "wb");
    if (!f) {
        perror("Can't open output file");
        goto cleanup;
    }

    fwrite(ddr_buffer, 1, TRANSFER_SIZE, f);
    fclose(f);
    printf("DMA data saved to /media/card/dma_dump.bin (%d bytes)\n", TRANSFER_SIZE);

cleanup:
    if (ddr_buffer && ddr_buffer != MAP_FAILED)
        munmap(ddr_buffer, TRANSFER_SIZE);
    if (dma_regs && dma_regs != MAP_FAILED)
        munmap((void*)dma_regs, 4096);
    if (fd >= 0)
        close(fd);
}
#endif

// int main() {
int main(int argc, char *argv[]) {
    printf("main start\n");
    // struct no_os_gpio_desc *gpio_resetb;
    // printf("testpoint 1\n");
    // struct no_os_gpio_init_param gpio_param = {
    //     .number = 973,  // заміни на свій GPIO
    //     .platform_ops = &linux_gpio_ops,
    //     .extra = "/dev/gpiochip0"
    // };

    // printf("testpoint 2\n");
    // if (no_os_gpio_get(&gpio_resetb, &gpio_param) < 0 ||
    //     no_os_gpio_direction_output(gpio_resetb, NO_OS_GPIO_HIGH) < 0) {
    //     printf("GPIO init failed\n");
    //     return -1;
    // }

    // printf("testpoint 3\n");


    // printf("testpoint 4\n");
    struct no_os_spi_desc *spi_desc;
    if (no_os_spi_init(&spi_desc, &spi_param) < 0) {
        printf("SPI init failed\n");
    return -1;
    }

    // printf("testpoint 5\n");

    // int id = ad9361_spi_read(spi_desc, REG_PRODUCT_ID); 
    // if (id < 0)
    //     printf("Помилка зчитування ID\n");
    // else
    //     printf("Product ID: 0x%02X\n", id);


    printf("testpoint 1\n");


    default_init_param.gpio_resetb.number = 973;
    default_init_param.gpio_sync.number = -1;
    default_init_param.gpio_cal_sw1.number = -1;
    default_init_param.gpio_cal_sw2.number = -1;
 



    struct ad9361_rf_phy *ad9361_phy;
    // ad9361_init(&ad9361_phy, &default_init_param);
    int ret = ad9361_init(&ad9361_phy, &default_init_param);
    if (ret < 0) {
        printf("ad9361_init failed: %d\n", ret);
        return -1;
    }

    // ad9361_spi_write(spi_desc, 0x010, 0xC0); // conf_1: default DDR
    // ad9361_spi_write(spi_desc, 0x011, 0x00); // conf_2: default
    // ad9361_spi_write(spi_desc, 0x012, 0x01); // conf_3: full port


    ad9361_set_tx_fir_config(ad9361_phy, tx_fir_config);
    ad9361_set_rx_fir_config(ad9361_phy, rx_fir_config);
 
    // ad9361_phy->pdata->ensm_pin_ctrl = false;
    // ad9361_phy->pdata->ensm_pin_pulse_mode = false; 

    // ad9361_set_en_state_machine_mode(ad9361_phy, ENSM_MODE_FDD);
    // usleep(10000);
    // ad9361_ensm_set_state(ad9361_phy, ENSM_STATE_ALERT, true); // use_spi = true
    // usleep(10000);
    // ad9361_ensm_set_state(ad9361_phy, ENSM_STATE_TX, true);    // use_spi = true
 


    // ad9361_bist_prbs(ad9361_phy, BIST_INJ_TX);

    // uint32_t val = ad9361_spi_read(spi_desc, REG_BIST_CONFIG);
    // printf("BIST CONFIG REG: 0x%08X\n", val);

    // uint32_t ensm_state = ad9361_spi_read(spi_desc, REG_STATE);
    // printf("ENSM state: 0x%02X\n", ensm_state);

    uint8_t port_conf = ad9361_spi_read(spi_desc, REG_PARALLEL_PORT_CONF_1); 
    printf("REG_PARALLEL_PORT_CONF_1: 0x%02X\n", port_conf);
    port_conf = ad9361_spi_read(spi_desc, REG_PARALLEL_PORT_CONF_2); 
    printf("REG_PARALLEL_PORT_CONF_2: 0x%02X\n", port_conf);
    port_conf = ad9361_spi_read(spi_desc, REG_PARALLEL_PORT_CONF_3); 
    printf("REG_PARALLEL_PORT_CONF_3: 0x%02X\n", port_conf);

    // ad9361_set_rx_gain_control_mode(ad9361_phy, 0, RF_GAIN_MGC);
	// ad9361_set_rx_rf_gain(ad9361_phy, 0, 0);

    // ad9361_set_rx_gain_control_mode(ad9361_phy, 1, RF_GAIN_MGC);
	// ad9361_set_rx_rf_gain(ad9361_phy, 1, 0);

	// ad9361_set_rx_gain_control_mode(ad9361_phy, 0, RF_GAIN_SLOWATTACK_AGC);
	// ad9361_set_rx_gain_control_mode(ad9361_phy, 1, RF_GAIN_SLOWATTACK_AGC);

	ad9361_set_rx_gain_control_mode(ad9361_phy, 0, RF_GAIN_FASTATTACK_AGC);
	ad9361_set_rx_gain_control_mode(ad9361_phy, 1, RF_GAIN_FASTATTACK_AGC);

	if (argc >= 2) {
    	uint64_t rx_freq = strtoull(argv[1], NULL, 10); // частота в Гц 
    	ad9361_set_rx_lo_freq(ad9361_phy, rx_freq);
    	printf("Set RX & TX frequency: %llu Hz\n", rx_freq);
	}
	
	if (argc >= 3) {
    	ad9361_set_rx_gain_control_mode(ad9361_phy, 0, RF_GAIN_MGC);
		int32_t rx_gain = (int32_t)strtol(argv[2], NULL, 10); // дБ
    	ad9361_set_rx_rf_gain(ad9361_phy, 0, rx_gain);
    	printf("Set RX0 GAIN: %d DB\n", rx_gain);
	}

    ad9361_set_rx_rf_bandwidth(ad9361_phy, 5000000); // 10 МГц
	ad9361_set_rx_sampling_freq(ad9361_phy, 15000000);

    sleep(1);

    struct rf_rssi rssi;
    int32_t gain;

	// struct axi_dmac *rx_dma;
    // int32_t status = axi_dmac_init(&rx_dma, &dma_init);
    // if (status < 0) {
    //     printf("DMA init failed!\n");
    //     return -1;
    // }

    // uint32_t *rx_buf = malloc(DMA_TRANSFER_LEN * sizeof(uint32_t));
    // if (!rx_buf) {
    //     printf("malloc failed\n");
    //     return -1;
    // }

	// // DMA Transfer
    // printf("Start DMA RX...\n");
    // status = dma_read_and_save(rx_dma, rx_buf, DMA_TRANSFER_LEN);
    // if (status < 0) {
    //     free(rx_buf);
    //     return -1;
    // }
    // printf("DMA RX finished! Saving to SD...\n");

	/*
	sleep(2);
	//dma_read_and_save();
	dma_proxy_read_and_save();
	sleep(2);
	*/
	// sleep(1);
	// dma_ddr_read_and_save();
	// sleep(1);

    while (true)
    {

		dma_ddr_read_and_save();
		usleep(100000);

        // if (ad9361_get_rx_rssi(ad9361_phy, 0, &rssi) == 0)
        //     printf("RX0 sym = %u, mult = %d", rssi.symbol, rssi.multiplier);
        //     // printf("RX0 RSSI: %.1f dB   ",(double)rssi.symbol * (double)rssi.multiplier / 1000.0);
        // else
        //     printf("Failed to read RX0 RSSI\n");

        // ad9361_get_rx_rf_gain(ad9361_phy, 0, &gain);
        // printf("Gain: %d dB\n", gain);
 
        // if (ad9361_get_rx_rssi(ad9361_phy, 1, &rssi) == 0)
        //     printf("RX1 sym = %u, mult = %d", rssi.symbol, rssi.multiplier);
        //     // printf("RX1 RSSI: %.1f dB\n", (double)rssi.symbol);
        //     // printf("RX1 RSSI: %.1f dB   ",(double)rssi.symbol * (double)rssi.multiplier / 1000.0);
        // else
        //     printf("Failed to read RX1 RSSI\n");

        // ad9361_get_rx_rf_gain(ad9361_phy, 1, &gain);
        // printf("Gain: %d dB\n", gain);

        // printf("while\n");
        // sleep(1);
    }

    

    sleep(1);

}