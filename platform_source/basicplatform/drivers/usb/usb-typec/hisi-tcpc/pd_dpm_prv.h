/*
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
 *
 * Copyright (C) 2016 Richtek Technology Corp.
 * Author: TH <tsunghan_tsai@richtek.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef PD_DPM_PRV_H_INCLUDED
#define PD_DPM_PRV_H_INCLUDED

typedef struct __eval_snk_request_result {
	int src_sel;
	int snk_sel;
} eval_snk_request_result_t;

#define SVID_DATA_LOCAL_MODE(svid_data, n) \
		svid_data->local_mode.mode_vdo[n]

#define SVID_DATA_REMOTE_MODE(svid_data, n) \
		svid_data->remote_mode.mode_vdo[n]

#define SVID_DATA_DFP_GET_ACTIVE_MODE(svid_data) \
		SVID_DATA_REMOTE_MODE((svid_data), (svid_data)->active_mode-1)

#define SVID_DATA_UFP_GET_ACTIVE_MODE(svid_data) \
		SVID_DATA_LOCAL_MODE((svid_data), (svid_data)->active_mode-1)

bool eval_snk_cap_request(
	const pd_port_power_caps *snk_caps,
	const pd_port_power_caps *src_caps,
	int strategy,
	eval_snk_request_result_t *result);

enum pd_ufp_u_state {
	DP_UFP_U_NONE = 0,
	DP_UFP_U_STARTUP,
	DP_UFP_U_WAIT,
	DP_UFP_U_OPERATION,
	DP_UFP_U_STATE_NR,

	DP_UFP_U_ERR = 0X10,

	DP_DFP_U_ERR_DP_CONNECTED,
};

typedef struct __pd_mode_prop {
	const char *name;
	uint32_t svid;
	void (*request_enter_mode)(pd_port_t *pd_port);
	void (*request_exit_mode)(pd_port_t *pd_port);
	bool (*dfp_inform_id)(pd_port_t *pd_port,
			pd_event_t *pd_event, bool ack);
	bool (*dfp_inform_svids)(pd_port_t *pd_port, bool ack);
	bool (*dfp_inform_modes)(pd_port_t *pd_port, bool ack);
	bool (*dfp_inform_enter_mode)(pd_port_t *pd_port, bool ack);
	bool (*dfp_inform_exit_mode)(pd_port_t *pd_port, uint16_t svid);
	bool (*dfp_inform_attention)(pd_port_t *pd_port, pd_event_t *pd_event);
	bool (*notify_pe_dfp_ready)(pd_port_t *pd_port, pd_event_t *pd_event);
	void (*reset_state)(pd_port_t *pd_port);
} pd_mode_prop_t;

extern void hisi_dpm_check_supported_modes(void);

typedef struct __svdm_svid_ops {
	const char *name;
	uint16_t svid;

	bool (*dfp_inform_id)(pd_port_t *pd_port, svdm_svid_data_t *svid_data,
			pd_event_t *pd_event, bool ack);
	bool (*dfp_inform_svids)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data, bool ack);
	bool (*dfp_inform_modes)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data, bool ack);

	bool (*dfp_inform_enter_mode)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data, uint8_t ops, bool ack);
	bool (*dfp_inform_exit_mode)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data, uint8_t ops);

	bool (*dfp_inform_attention)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data, pd_event_t *pd_event);

	void (*ufp_request_enter_mode)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data, uint8_t ops);
	void (*ufp_request_exit_mode)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data, uint8_t ops);

	bool (*notify_pe_startup)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data);
	int (*notify_pe_ready)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data, pd_event_t *pd_event);
	bool (*notify_pe_shutdown)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data);

#ifdef CONFIG_USB_PD_UVDM_SUPPORT
	bool (*dfp_notify_uvdm)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data, bool ack);
	bool (*ufp_notify_uvdm)(pd_port_t *pd_port,
			svdm_svid_data_t *svid_data);
#endif
	bool (*reset_state)(pd_port_t *pd_port, svdm_svid_data_t *svid_data);
} svdm_svid_ops_t;

#ifdef CONFIG_USB_PD_REV30
static inline bool dpm_check_ext_msg_event(pd_event_t *pd_event, uint8_t msg)
{
	return pd_event_ext_msg_match(pd_event, msg);
}
#endif

static inline svdm_svid_data_t *dpm_get_svdm_svid_data(
		pd_port_t *pd_port, uint16_t svid)
{
	uint8_t i;
	svdm_svid_data_t *svid_data = NULL;

	D("+\n");
	if (!(pd_port->id_vdos[0] & PD_IDH_MODAL_SUPPORT))
		return NULL;

	for (i = 0; i < pd_port->svid_data_cnt; i++) {
		svid_data = &pd_port->svid_data[i];
		if (svid_data->svid == svid)
			return svid_data;
	}
	D("-\n");

	return NULL;
}

static inline void dpm_vdm_get_svid_ops(
		pd_event_t *pd_event, uint16_t *svid, uint8_t *ops)
{
	uint32_t vdm_hdr;

	if (!pd_event->pd_msg)
		return;

	vdm_hdr = pd_event->pd_msg->payload[0];
	if (svid)
		*svid = PD_VDO_VID(vdm_hdr);
	if (ops)
		*ops = PD_VDO_OPOS(vdm_hdr);
}

static inline bool dpm_register_svdm_ops(pd_port_t *pd_port,
		const svdm_svid_ops_t *ops)
{
	svdm_svid_data_t *svid_data = NULL;

	D("+\n");
	svid_data = dpm_get_svdm_svid_data(pd_port, ops->svid);
	if (!svid_data)
		return false;

	svid_data->ops = ops;
	D("-\n");
	return true;
}

static inline bool svdm_notify_pe_startup(pd_port_t *pd_port)
{
	uint8_t i;
	svdm_svid_data_t *svid_data = NULL;

	D("+\n");
	for (i = 0; i < pd_port->svid_data_cnt; i++) {
		svid_data = &pd_port->svid_data[i];
		/* hisi_dp_dfp_u_notify_pe_startup */
		if (svid_data->ops && svid_data->ops->notify_pe_startup)
			svid_data->ops->notify_pe_startup(pd_port, svid_data);
	}

	D("-\n");
	return true;
}

static inline int svdm_notify_pe_ready(pd_port_t *pd_port, pd_event_t *pd_event)
{
	int ret;
	uint8_t i;
	svdm_svid_data_t *svid_data = NULL;

	D("+\n");
	for (i = 0; i < pd_port->svid_data_cnt; i++) {
		svid_data = &pd_port->svid_data[i];
		if (svid_data->ops && svid_data->ops->notify_pe_ready) {
			ret = svid_data->ops->notify_pe_ready(pd_port,
					svid_data, pd_event);
			if (ret != 0)
				return ret;
		}
	}

	D("-\n");
	return 0;
}

static inline bool svdm_notify_pe_shutdown(pd_port_t *pd_port)
{
	uint8_t i;
	svdm_svid_data_t *svid_data = NULL;

	for (i = 0; i < pd_port->svid_data_cnt; i++) {
		svid_data = &pd_port->svid_data[i];
		if (svid_data->ops && svid_data->ops->notify_pe_shutdown)
			svid_data->ops->notify_pe_shutdown(pd_port, svid_data);
	}

	return 0;
}

static inline bool svdm_reset_state(pd_port_t *pd_port)
{
	uint8_t i;
	svdm_svid_data_t *svid_data = NULL;

	pd_port->dpm_charging_policy = pd_port->dpm_charging_policy_default;
	for (i = 0; i < pd_port->svid_data_cnt; i++) {
		svid_data = &pd_port->svid_data[i];
		if (svid_data->ops && svid_data->ops->reset_state)
			svid_data->ops->reset_state(pd_port, svid_data);
	}

	return true;
}


static inline bool svdm_dfp_inform_id(
	pd_port_t *pd_port, pd_event_t *pd_event, bool ack)
{
	uint8_t i;
	svdm_svid_data_t *svid_data = NULL;

	for (i = 0; i < pd_port->svid_data_cnt; i++) {
		svid_data = &pd_port->svid_data[i];
		if (svid_data->ops && svid_data->ops->dfp_inform_id)
			svid_data->ops->dfp_inform_id(pd_port,
					svid_data, pd_event, ack);
	}

	return true;
}

static inline bool svdm_dfp_inform_svids(pd_port_t *pd_port, bool ack)
{
	uint8_t i;
	svdm_svid_data_t *svid_data = NULL;

	for (i = 0; i < pd_port->svid_data_cnt; i++) {
		svid_data = &pd_port->svid_data[i];
		if (svid_data->ops && svid_data->ops->dfp_inform_svids)
			svid_data->ops->dfp_inform_svids(pd_port,
					svid_data, ack);
	}

	return true;
}

static inline bool svdm_dfp_inform_modes(pd_port_t *pd_port,
		uint16_t svid, bool ack)
{
	svdm_svid_data_t *svid_data = dpm_get_svdm_svid_data(pd_port, svid);

	if (!svid_data)
		return false;

	if (svid_data->ops && svid_data->ops->dfp_inform_modes)
		svid_data->ops->dfp_inform_modes(pd_port, svid_data, ack);

	return true;
}

static inline bool svdm_dfp_inform_enter_mode(pd_port_t *pd_port,
		uint16_t svid, uint8_t ops, bool ack)
{
	svdm_svid_data_t *svid_data = dpm_get_svdm_svid_data(pd_port, svid);

	if (!svid_data)
		return false;

	if (svid_data->ops && svid_data->ops->dfp_inform_enter_mode)
		svid_data->ops->dfp_inform_enter_mode(pd_port,
				svid_data, ops, ack);

	return true;
}

static inline bool svdm_dfp_inform_exit_mode(pd_port_t *pd_port,
		uint16_t svid, uint8_t ops)
{
	svdm_svid_data_t *svid_data = dpm_get_svdm_svid_data(pd_port, svid);

	if (!svid_data)
		return false;

	if (svid_data->ops && svid_data->ops->dfp_inform_exit_mode)
		svid_data->ops->dfp_inform_exit_mode(pd_port, svid_data, ops);

	return true;
}

static inline bool svdm_dfp_inform_attention(
		pd_port_t *pd_port, uint16_t svid, pd_event_t *pd_event)
{
	svdm_svid_data_t *svid_data = dpm_get_svdm_svid_data(pd_port, svid);

	if (!svid_data)
		return false;

	if (svid_data->ops && svid_data->ops->dfp_inform_attention)
		svid_data->ops->dfp_inform_attention(pd_port,
				svid_data, pd_event);

	return true;
}

static inline bool svdm_ufp_request_enter_mode(pd_port_t *pd_port,
		uint16_t svid, uint8_t ops)
{
	svdm_svid_data_t *svid_data = dpm_get_svdm_svid_data(pd_port, svid);

	if (!svid_data)
		return false;

	if (svid_data->ops && svid_data->ops->ufp_request_enter_mode)
		svid_data->ops->ufp_request_enter_mode(pd_port, svid_data, ops);

	return true;
}

static inline bool svdm_ufp_request_exit_mode(pd_port_t *pd_port,
		uint16_t svid, uint8_t ops)
{
	svdm_svid_data_t *svid_data = dpm_get_svdm_svid_data(pd_port, svid);

	if (!svid_data)
		return false;

	if (svid_data->ops && svid_data->ops->ufp_request_exit_mode)
		svid_data->ops->ufp_request_exit_mode(pd_port, svid_data, ops);

	return true;
}

#endif /* PD_DPM_PRV_H_INCLUDED */
