#ifndef UI_RESOURCE_CODE_H__
#define UI_RESOURCE_CODE_H__

/* ------------------------------------------------------------------ */
/* Platform includes                                                    */
/* On Zephyr these types are injected by the build system.             */
/* On Silicon Labs GSDK they must be pulled in explicitly.             */
/* ------------------------------------------------------------------ */
#include <stdint.h>    /* uint8_t, uint16_t, uint32_t, int8_t … */
#include <stddef.h>    /* ptrdiff_t, size_t */
#include <limits.h>    /* UINT8_MAX */

#if defined(__ZEPHYR__)
#  include <zephyr/bluetooth/bluetooth.h>   /* bt_addr_le_t */
#  include <zephyr/sys/iterable_sections.h> /* TYPE_SECTION_ITERABLE */
#else
/* Silicon Labs GSDK — compat header provides bt_addr_le_t and          */
/* TYPE_SECTION_ITERABLE (identical GCC section-attribute mechanism).   */
#  include "silabs_nordic_compat.h"
#endif

// man_id
#ifndef MANUFACTURER_ID
  // #define MANUFACTURER_ID   ((uint16_t)0x0059)
  #define MANUFACTURER_ID    ((uint16_t)0xFFFF)
#endif

// form_id
#ifndef LOSS_TEST_FORM_ID
  #define LOSS_TEST_FORM_ID ((uint16_t)0xBAAB)
#endif

#define LOSS_TEST_REMOTE_CTRL_TERM_ID	(uint16_t)('R'+('T'<<8)) // controller side
#define LOSS_TEST_REMOTE_CTRL_TERM_ACT	(uint16_t)('r'+('t'<<8)) // tester side

#define LOSS_TEST_REMOTE_CTRL_KEYPAD_ID	(uint16_t)('R'+('C'<<8)) // controller side
#define LOSS_TEST_REMOTE_CTRL_KEYPAD_ACT	(uint16_t)('r'+('c'<<8)) // tester side

#define LOSS_TEST_REMOTE_MONITOR_ID (uint16_t)('R'+('M'<<8)) // monitor side
#define LOSS_TEST_REMOTE_MONITOR_ACT (uint16_t)('r'+('m'<<8)) // tester side

// ctrl_id
//#define RESOURCE_LIST_DNSTR			((uint8_T)'R') // tester's resource stream
//#define UI_LAYOUT					((uint8_T)'S') // remote-controller's UI-layout / UI-scene
//#define UI_SCENE					((uint8_T)'s') // remote-controller's UI-layout / UI-scene
//#define RESOURCE_TOGGLE				((uint8_T)'v') // remote-controller change-value
//#define RESOURCE_VALUE_DNSTR		((uint8_T)'V') // tester's value-update
//#define RESOURCE_VALUE_UPSTR		((uint8_T)'v') // remote-controller enquire/change-value

#define ASCSTR_2_UINT32(arg_a,arg_b,arg_c,arg_d) (arg_a +(arg_b<<8) +(arg_c<<16) +(arg_d<<24))

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

typedef enum{
	RTYP_IDX_FORMSTR=	0x0F, // stdio form string, printf(form,...)
	RTYP_IDX_DEVCTRL=	0x0E, // device control
	RTYP_IDX_CHARSTR=	0x00, // c-string
	RTYP_IDX_REDIR=		0x09, // c-string
	RTYP_IDX_UINT8=		0x01, // uint8_t series
	RTYP_IDX_UINT16=	0x02, // uint16_t series
	RTYP_IDX_UINT32=	0x03, // uint32_t series
	RTYP_IDX_INDIRECT=	0x04, // resource / c-string index
	RTYP_IDX_INT8=		0x05, // int8_t series
	RTYP_IDX_INT16=		0x06, // int16_t series
	RTYP_IDX_INT32=		0x07 // int32_t series
} RTYP_IDX_NUM;

#define RES_BTN_HLD			1 // for extrn SCR only
#define RES_BTN_1			2
#define RES_BTN_2			4
#define RES_BTN_3			6
#define RES_BTN_4			8
#define RES_BTN_5			10
#define RES_BTN_6			12
#define RES_BTN_7			14
#define RES_BTN_8			16
#define RES_BTN_9			18
#define RES_BTN_10			20
#define RES_BTN_11			22
#define RES_BTN_12			24
#define RES_BTN_13			26
#define RES_BTN_14			28
#define RES_BTN_15			30

#define RES_SCALE_5000		0b01110	// ScaledValue = value * 5 * pow(10,3)
#define RES_SCALE_2000		0b01101	// ScaledValue = value * 2 * pow(10,3)
#define RES_SCALE_1000		0b01100	// ScaledValue = value * 1 * pow(10,3)
#define RES_SCALE_500		0b01010	// ScaledValue = value * 5 * pow(10,2)
#define RES_SCALE_200		0b01001	// ScaledValue = value * 2 * pow(10,2)
#define RES_SCALE_100		0b01000	// ScaledValue = value * 1 * pow(10,2)
#define RES_SCALE_50		0b00110	// ScaledValue = value * 5 * pow(10,1)
#define RES_SCALE_20		0b00101	// ScaledValue = value * 2 * pow(10,1)
#define RES_SCALE_10		0b00100	// ScaledValue = value * 1 * pow(10,1)
#define RES_SCALE_1			0		// ScaledValue = value * 1 * pow(10,0)
#define RES_SCALE_2			1		// ScaledValue = value * 2 * pow(10,0)
#define RES_SCALE_5			2		// ScaledValue = value * 5 * pow(10,0)
#define RES_SCALE_1_10th	0b11100	// ScaledValue = value * 1 * pow(10,-1)
#define RES_SCALE_2_10th	0b11110	// ScaledValue = value * 5 * pow(10,-1)
#define RES_SCALE_5_10th	0b11101	// ScaledValue = value * 2 * pow(10,-1)
#define RES_SCALE_1_100th	0b11000	// ScaledValue = value * 1 * pow(10,-2)
#define RES_SCALE_2_100th	0b11010	// ScaledValue = value * 5 * pow(10,-2)
#define RES_SCALE_5_100th	0b11001	// ScaledValue = value * 2 * pow(10,-2)
#define RES_SCALE_1_1000th	0b10100	// ScaledValue = value * 1 * pow(10,-3)
#define RES_SCALE_2_1000th	0b10110	// ScaledValue = value * 5 * pow(10,-3)
#define RES_SCALE_5_1000th	0b10101	// ScaledValue = value * 2 * pow(10,-3)
#define RES_SCALE_1_10000th	0b10000	// ScaledValue = value * 1 * pow(10,-4)
#define RES_SCALE_2_10000th	0b10010	// ScaledValue = value * 5 * pow(10,-4)
#define RES_SCALE_5_10000th	0b10001	// ScaledValue = value * 2 * pow(10,-4)

typedef union __attribute__((packed)) {
	uint16_t typ_raw;
	struct __attribute__((packed)) {
		unsigned typ_idx:5;
		unsigned 		:1;
		unsigned scale	:5;
		unsigned btn_hdl:5; // (1) external scr : 2..31 ; (2) smart phone : 1,31
	};
	struct __attribute__((packed)) {
		unsigned 		 :6;
		unsigned scale_a :2;
		signed   scale_b :3;
		unsigned btn_hld :1;
		unsigned btn_idx :4;
	};
} RESOURCE_TYPE_ST;

typedef struct __attribute__((packed)) {
	uint16_t res_idx;
	RESOURCE_TYPE_ST type_info;
	uint8_t sz;
	void *  resource_p;
} RESOURCE_ST;

typedef struct __attribute__((packed)) {
	uint16_t res_idx;
	RESOURCE_TYPE_ST type_info;
	uint8_t sz;
	void *  resource_p;
	void (*evt_handle)();
	const void *  tbl_p;
	const uint8_t tbl_len;
  #if defined(RESLST_CTOR_wiSYMSTR)
	const char * nm_p;
  #endif
} RESOURCE_xST;

typedef struct __attribute__((packed)) {
	uint16_t res_idx;
	RESOURCE_TYPE_ST type_info;
	uint8_t sz;
	union __attribute__((__packed__)) {
		int8_t   i8[0];//[12];
		int16_t  i16[0];//[6];
		int32_t  i32[0];//[3];
		uint8_t  u8[0];//[12];
		uint16_t u16[0];//[6];
		uint32_t u32[0];//[3];
		char     cstr[0];//[180];
	};
} RESOURCE_CONTEXT_SECTION_ST;

typedef struct __attribute__((__packed__)) {
	uint16_t res_idx; // resource index
	union __attribute__((__packed__)) {
		int8_t   i8[0];//[12];
		int16_t  i16[0];//[6];
		int32_t  i32[0];//[3];
		uint8_t  u8[0];//[12];
		uint16_t u16[0];//[6];
		uint32_t u32[0];//[3];
	};
} RESOURCE_VALUE_FIELD_ST;

typedef struct __attribute__((__packed__)) {
	uint16_t res_idx; // resource index
	uint8_t elem_idx;
} RESOURCE_SCENE_ELEM_ST;

typedef enum{
	// RC/RT/RT to tester
	RCREQ_ENQ_NONE=0,
	RCREQ_ENQ_RESOURCE_CONTEXT=1,	// resp : RCRSP_RSP_RESOURCE_CONTEXT, RCRSP_NAK
	RCREQ_ENQ_SCENE_CONTEXT=2,		// resp : RCRSP_RSP_SCENE_CONTEXT, RCRSP_NAK
	RCREQ_GET_VALUE=3,				// resp : RCRSP_RSP_VALUE, RCRSP_NAK
	RCREQ_ENQ_LIVE=4,				// resp : RC_BRCAST_SCENE, RCRSP_RSP_SCENE
	RCREQ_CHG_VALUE=5,				// resp : RCRSP_RSP_VALUE, RCRSP_NAK, RCRSP_RSP_SCENE
	RMREQ_ENQ_LIVE=6,				// resp : RM_BRCAST_VALUE
	RCREQ_ENQ_CONTACT=14,			// resp : RCRSP_ACK_CONTACT, RCRSP_NAK_CONTACT
	RCREQ_ENQ_RELEASE=15,			// resp : RCRSP_NAK_CONTACT

	// tester to RC/RT
	RCRSP_NAK=0,
	RCRSP_RSP_RESOURCE_CONTEXT=33,
	RCRSP_RSP_SCENE_CONTEXT=34,
	RCRSP_RSP_SCENE=35,
	RCRSP_RSP_VALUE=36,
	RC_BRCAST_SCENE=37,
	RM_BRCAST_VALUE=38,				// RM rush message
	RCRSP_RLS_CONTACT=44,			// notify RC/RT that the contact is released
	RCRSP_NAK_OCCUPIED=45,
	RCRSP_ACK_CONTACT=46,
	RCRSP_NAK_CONTACT=47,

	// inner 
	RCEVT_ADV_STOP=48,
	RCEVT_ADV_SENT,
	RCEVT_ADV_ARRIVES,
	RCEVT_RM_BRCAST,
	RCEVT_BTN_CLICK,
	RCEVT_BTN_HOLD,
	RCEVT_CONNECT,
	RCEVT_DISCONNECT,
	RCEVT_ESCAPE,
	TMOUT_ENQ_RELEASE
}RC_CMD_t;

typedef struct __attribute__((__packed__)) {
	int8_t ser_cnt; // series count / flow conut
	uint8_t ctrl_idx; // resource index
	union {
		// remote control side
		uint8_t request;
						
		// tester side
		uint8_t respond;
	};
} RESOURCE_CTRL_ST;

typedef struct __attribute__((__packed__)) {
	uint16_t man_id;
	uint16_t form_id; // rc code
	uint32_t node_id;
	RESOURCE_CTRL_ST ctrl_info;
	union __attribute__((__packed__)) {
		bt_addr_le_t                address[0];
		RESOURCE_VALUE_FIELD_ST     res_field[0];
		RESOURCE_CONTEXT_SECTION_ST res_context[0];
		RESOURCE_SCENE_ELEM_ST	    scene_elem[0];
		uint8_t                     raw_data[213];//[150];
	};
} DEV_RESOURCE_MSG_FRAME_ST;

typedef struct __attribute__((__packed__)) {
	uint16_t man_id;
	uint16_t form_id; // rc code
	uint32_t node_id;
	RESOURCE_CTRL_ST ctrl_info;
} DEV_RESOURCE_HEAD_ST;

typedef struct __attribute__((__packed__)) {
	RESOURCE_CTRL_ST         ctrl_info;
	union __attribute__((__packed__)) {
		RESOURCE_VALUE_FIELD_ST     res_field[0];
		RESOURCE_CONTEXT_SECTION_ST res_context[0];
		RESOURCE_SCENE_ELEM_ST	    scene_elem[0];
		uint8_t                     raw_data[213];//[150];
	};
} RESOURCE_STREAM_ST;



// ORIG side func/macro

typedef uint8_t(*resource_get_ui8)(void);
typedef uint16_t(*resource_get_ui16)(void);
typedef uint32_t(*resource_get_ui32)(void);
typedef int8_t(*resource_get_si8)(void);
typedef int16_t(*resource_get_si16)(void);
typedef int32_t(*resource_get_si32)(void);

#define RLST_IDX(arg_nm) RLST_IDX_##arg_nm

#if defined(RESLST_CTOR_wiSYMSTR)
  #define RLST_REF(arg_nm)  .res_idx=RLST_IDX(arg_nm), .nm_p=#arg_nm
#else
  #define RLST_REF(arg_nm)  .res_idx=RLST_IDX(arg_nm)
#endif

#define RLST_NM(arg_nm)  static const char * RLST_NM_##arg_nm= #arg_nm

#define SCENE_FROM(arg)  {.res_idx=RLST_IDX(arg), .elem_idx=UINT8_MAX}
#define SCENE_ARGS(arg1,arg2)  {.res_idx=RLST_IDX(arg1), .elem_idx=arg2}
#define SCENE_END  {.res_idx=UINT16_MAX, .elem_idx=UINT8_MAX}

#define RSRC_ENUM_VAL(arg) \
	((*(0+(resource_get_ui8 *)((FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p)))())

#define RSRC_ENUM_CSTR(arg) \
	(FixedRsrc +resource_sort[RSRC_ENUM_VAL(arg)])->resource_p

#define RSRC_FIXED_CSTR(arg) \
	(FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p

#define RSRC_REDIR_CSTR(arg) \
	(RTYP_IDX_REDIR==(FixedRsrc +resource_sort[RLST_IDX(arg)])->type_info.typ_idx)? \
		(FixedRsrc +resource_sort[(uint32_t)((FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p)])->resource_p \
		:(FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p

#define RSRC_UI8_SERIES(arg,series) \
	(*(series+(resource_get_ui8 *)((FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p)))()

#define RSRC_SI8_SERIES(arg,series) \
	(*(series+(resource_get_si8 *)((FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p)))()

#define RSRC_UI16_SERIES(arg,series) \
	(*(series+(resource_get_ui16 *)((FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p)))()

#define RSRC_SI16_SERIES(arg,series) \
	(*(series+(resource_get_si16 *)((FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p)))()

#define RSRC_UI32_SERIES(arg,series) \
	(*(series+(resource_get_ui32 *)((FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p)))()

#define RSRC_SI32_SERIES(arg,series) \
	(*(series+(resource_get_si32 *)((FixedRsrc +resource_sort[RLST_IDX(arg)])->resource_p)))()

//Constructor //CTOR
//Destructor //DTOR

// Constructor
// Use enum so the value is an integer constant expression in C,
// allowing it to appear in static struct initializers.
#if(__RES_CONSTANT_DECL__)
  #define RLST_STP3(arg_nm) extern const uint8_t arg_nm ;
#else
  #define RLST_STP3(arg_nm) enum { arg_nm = __COUNTER__ };
#endif

#define RLST_CTOR_STP2(arg_nm) RLST_STP3(RLST_IDX_##arg_nm)
#define RLST_CTOR_STP1(arg_typ,arg_ridtyp,arg_nm,sec_nm)


#if(__RES_CONSTANT_DECL__)

  #define RLST_ID0(arg_typ,arg_typid,arg_nm,sec_nm)  RLST_CTOR_STP2(arg_nm)
  #define RLST(arg_typ,arg_typid,arg_nm,sec_nm)  RLST_CTOR_STP2(arg_nm)
  #define RLST_CSTR(arg_typ,arg_nm,sec_nm)  RLST_CTOR_STP2(arg_nm)
  #define RLST_CSTR_HDL(arg_typ,arg_nm,arg_hdl,sec_nm,hdl_func)  RLST_CTOR_STP2(arg_nm) 
  #define RLST_REDIR_CSTR(arg_typ,arg_nm,arg_redir,sec_nm)  RLST_CTOR_STP2(arg_nm)
  #define RLST_REDIR_CSTR_HDL(arg_typ,arg_nm,arg_redir,arg_hdl,sec_nm,hdl_func)  RLST_CTOR_STP2(arg_nm)
  #define RLST_FORMSTR(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP2(arg_nm)
  #define RLST_DEVCTRL(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_FORMSTR,arg_nm,sec_nm)
  #define RLST_INDIR(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP2(arg_nm)
  #define RLST_INDIR_HDL(arg_typ,arg_nm,arg_hdl,sec_nm,hdl_func) RLST_CTOR_STP2(arg_nm)
  #define RLST_SI8(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP2(arg_nm)
  #define RLST_UI8(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP2(arg_nm)
  #define RLST_SI16(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP2(arg_nm)
  #define RLST_SI16_HDL(arg_typ,arg_nm,arg_hdl,sec_nm,hdl_func) RLST_CTOR_STP2(arg_nm)
  #define RLST_SI16_SCALED(arg_typ,arg_nm,arg_scale,sec_nm) RLST_CTOR_STP2(arg_nm)
  #define RLST_UI16(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP2(arg_nm)
  #define RLST_UI16_SCALED(arg_typ,arg_nm,arg_scale,sec_nm) RLST_CTOR_STP2(arg_nm)
  #define RLST_SI32(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP2(arg_nm)
  #define RLST_UI32(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP2(arg_nm)

#elif(1)

  #define RLST_ID0(arg_typ,arg_typid,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,arg_typid,arg_nm,sec_nm) \
	const uint8_t RLST_IDX_##arg_nm = 0; \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
	{.type_info.typ_idx=arg_typid, RLST_REF(arg_nm), \
		.sz=sizeof(arg_nm), \
		.resource_p=(char *)arg_nm };


//  #define RLST(arg_typ,arg_typid,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,arg_typid,arg_nm,sec_nm)
//	RLST_CTOR_STP2(arg_nm)
//	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm )


  #define RLST_CSTR(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_CHARSTR,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_CHARSTR, RLST_REF(arg_nm), \
			.sz=sizeof(arg_nm), \
			.resource_p=(char *)arg_nm };


  #define RLST_CSTR_HDL(arg_typ,arg_nm,arg_hdl,sec_nm,hdl_func) RLST_CTOR_STP1(arg_typ,RTYP_IDX_CHARSTR,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_CHARSTR, .type_info.btn_hdl=arg_hdl, RLST_REF(arg_nm), \
			.sz=sizeof(arg_nm), \
			.resource_p=(char *)arg_nm, \
			.evt_handle=hdl_func };


  #define RLST_REDIR_CSTR(arg_typ,arg_nm,arg_redir,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_CHARSTR,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_REDIR, RLST_REF(arg_nm), \
			.sz=1, \
			.resource_p=(ptrdiff_t)RLST_IDX_##arg_redir };


  #define RLST_REDIR_CSTR_HDL(arg_typ,arg_nm,arg_redir,arg_hdl,sec_nm,hdl_func) RLST_CTOR_STP1(arg_typ,RTYP_IDX_CHARSTR,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_REDIR,.type_info.btn_hdl=arg_hdl, RLST_REF(arg_nm), \
			.sz=1, \
			.resource_p=(void *)((ptrdiff_t)RLST_IDX_##arg_redir), \
			.evt_handle=hdl_func };


  #define RLST_FORMSTR(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_FORMSTR,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_FORMSTR, RLST_REF(arg_nm), \
			.sz=sizeof(arg_nm), \
			.resource_p=(char *)arg_nm };
		

  #define RLST_DEVCTRL(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_FORMSTR,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_DEVCTRL, RLST_REF(arg_nm), \
			.sz=sizeof(arg_nm), \
			.resource_p=(char *)arg_nm };


  #define RLST_INDIR(arg_typ,arg_nm,tbl_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_INT8,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_INDIRECT, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm, \
			.tbl_p=tbl_nm, \
			.tbl_len=sizeof(tbl_nm)/sizeof(tbl_nm[0]) };


  #define RLST_INDIR_HDL(arg_typ,arg_nm,tbl_nm,arg_hdl,sec_nm,hdl_func) RLST_CTOR_STP1(arg_typ,RTYP_IDX_INT8,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_INDIRECT, .type_info.btn_hdl=arg_hdl, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm, \
			.evt_handle=hdl_func, \
			.tbl_p=tbl_nm,.tbl_len=sizeof(tbl_nm)/sizeof(tbl_nm[0]) };


  #define RLST_SI8(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_INT8,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_INT8, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm };
		

  #define RLST_UI8(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_UINT8,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_UINT8, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm };


  #define RLST_SI16(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_INT16,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_INT16, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm };


  #define RLST_SI16_HDL(arg_typ,arg_nm,arg_hdl,sec_nm,hdl_func) RLST_CTOR_STP1(arg_typ,RTYP_IDX_INT16,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_INT16, .type_info.btn_hdl=arg_hdl, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm, \
			.evt_handle=hdl_func };


  #define RLST_SI16_SCALED(arg_typ,arg_nm,arg_scale,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_INT16,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_INT16, .scale=arg_scale, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm };


  #define RLST_UI16(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_UINT16,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_UINT16, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm };


  #define RLST_UI16_SCALED(arg_typ,arg_nm,arg_scale,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_UINT16,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_UINT16, .type_info.scale=arg_scale, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm };


  #define RLST_SI32(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_INT32,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_INT32, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm };


  #define RLST_UI32(arg_typ,arg_nm,sec_nm) RLST_CTOR_STP1(arg_typ,RTYP_IDX_UINT32,arg_nm,sec_nm) \
	RLST_CTOR_STP2(arg_nm) \
	static const TYPE_SECTION_ITERABLE(arg_typ,RLST_##arg_nm,sec_nm,) = \
		{.type_info.typ_idx=RTYP_IDX_UINT32, RLST_REF(arg_nm), \
			.sz=ARRAY_SIZE(arg_nm), \
			.resource_p=(char *)arg_nm };
		
#endif

#define IS_RTYP_INTIGER(arg) \
	(RTYP_IDX_UINT8==arg || RTYP_IDX_INT8==arg || \
	RTYP_IDX_UINT16==arg || RTYP_IDX_INT16==arg || \
	RTYP_IDX_UINT32==arg || RTYP_IDX_INT32==arg)

#define IS_RTYP_SIGNED(arg) \
	(RTYP_IDX_INT8==arg || RTYP_IDX_INT16==arg || RTYP_IDX_INT32==arg)

#define IS_RTYP_UNSIGNED(arg) \
	(RTYP_IDX_UINT8==arg || RTYP_IDX_UINT16==arg || RTYP_IDX_UINT32==arg)

#define IS_RTYP_INTIGER_8(arg) \
	(RTYP_IDX_UINT8==arg || RTYP_IDX_INT8==arg)

#define IS_RTYP_INTIGER_16(arg) \
	(RTYP_IDX_UINT16==arg || RTYP_IDX_INT16==arg)

#define IS_RTYP_INTIGER_32(arg) \
	(RTYP_IDX_UINT32==arg || RTYP_IDX_INT32==arg)

#define IS_RTYP_CHAR_STR(arg) \
	(RTYP_IDX_CHARSTR==arg || RTYP_IDX_FORMSTR==arg || RTYP_IDX_DEVCTRL==arg)

typedef union __attribute__((__packed__)) {
	const char * res_name[256];
	struct __attribute__((__packed__)) {
		const char * res_nm_0;
		const char * res_nm_1;
		const char * res_nm_2;
		const char * res_nm_3;
		const char * res_nm_4;
		const char * res_nm_5;
		const char * res_nm_6;
		const char * res_nm_7;
		const char * res_nm_8;
		const char * res_nm_9;
		const char * res_nm_10;
		const char * res_nm_11;
		const char * res_nm_12;
		const char * res_nm_13;
		const char * res_nm_14;
		const char * res_nm_15;
		const char * res_nm_16;
		const char * res_nm_17;
		const char * res_nm_18;
		const char * res_nm_19;
		const char * res_nm_20;
		const char * res_nm_21;
		const char * res_nm_22;
		const char * res_nm_23;
		const char * res_nm_24;
		const char * res_nm_25;
		const char * res_nm_26;
		const char * res_nm_27;
		const char * res_nm_28;
		const char * res_nm_29;
		const char * res_nm_30;
		const char * res_nm_31;
		const char * res_nm_32;
		const char * res_nm_33;
		const char * res_nm_34;
		const char * res_nm_35;
		const char * res_nm_36;
		const char * res_nm_37;
		const char * res_nm_38;
		const char * res_nm_39;
		const char * res_nm_40;
		const char * res_nm_41;
		const char * res_nm_42;
		const char * res_nm_43;
		const char * res_nm_44;
		const char * res_nm_45;
		const char * res_nm_46;
		const char * res_nm_47;
		const char * res_nm_48;
		const char * res_nm_49;
		const char * res_nm_50;
		const char * res_nm_51;
		const char * res_nm_52;
		const char * res_nm_53;
		const char * res_nm_54;
		const char * res_nm_55;
		const char * res_nm_56;
		const char * res_nm_57;
		const char * res_nm_58;
		const char * res_nm_59;
		const char * res_nm_60;
		const char * res_nm_61;
		const char * res_nm_62;
		const char * res_nm_63;
		const char * res_nm_64;
		const char * res_nm_65;
		const char * res_nm_66;
		const char * res_nm_67;
		const char * res_nm_68;
		const char * res_nm_69;
		const char * res_nm_70;
		const char * res_nm_71;
		const char * res_nm_72;
		const char * res_nm_73;
		const char * res_nm_74;
		const char * res_nm_75;
		const char * res_nm_76;
		const char * res_nm_77;
		const char * res_nm_78;
		const char * res_nm_79;
		const char * res_nm_80;
		const char * res_nm_81;
		const char * res_nm_82;
		const char * res_nm_83;
		const char * res_nm_84;
		const char * res_nm_85;
		const char * res_nm_86;
		const char * res_nm_87;
		const char * res_nm_88;
		const char * res_nm_89;
		const char * res_nm_90;
		const char * res_nm_91;
		const char * res_nm_92;
		const char * res_nm_93;
		const char * res_nm_94;
		const char * res_nm_95;
		const char * res_nm_96;
		const char * res_nm_97;
		const char * res_nm_98;
		const char * res_nm_99;
		const char * res_nm_100;
		const char * res_nm_101;
		const char * res_nm_102;
		const char * res_nm_103;
		const char * res_nm_104;
		const char * res_nm_105;
		const char * res_nm_106;
		const char * res_nm_107;
		const char * res_nm_108;
		const char * res_nm_109;
		const char * res_nm_110;
		const char * res_nm_111;
		const char * res_nm_112;
		const char * res_nm_113;
		const char * res_nm_114;
		const char * res_nm_115;
		const char * res_nm_116;
		const char * res_nm_117;
		const char * res_nm_118;
		const char * res_nm_119;
		const char * res_nm_120;
		const char * res_nm_121;
		const char * res_nm_122;
		const char * res_nm_123;
		const char * res_nm_124;
		const char * res_nm_125;
		const char * res_nm_126;
		const char * res_nm_127;
		const char * res_nm_128;
		const char * res_nm_129;
		const char * res_nm_130;
		const char * res_nm_131;
		const char * res_nm_132;
		const char * res_nm_133;
		const char * res_nm_134;
		const char * res_nm_135;
		const char * res_nm_136;
		const char * res_nm_137;
		const char * res_nm_138;
		const char * res_nm_139;
		const char * res_nm_140;
		const char * res_nm_141;
		const char * res_nm_142;
		const char * res_nm_143;
		const char * res_nm_144;
		const char * res_nm_145;
		const char * res_nm_146;
		const char * res_nm_147;
		const char * res_nm_148;
		const char * res_nm_149;
		const char * res_nm_150;
		const char * res_nm_151;
		const char * res_nm_152;
		const char * res_nm_153;
		const char * res_nm_154;
		const char * res_nm_155;
		const char * res_nm_156;
		const char * res_nm_157;
		const char * res_nm_158;
		const char * res_nm_159;
		const char * res_nm_160;
		const char * res_nm_161;
		const char * res_nm_162;
		const char * res_nm_163;
		const char * res_nm_164;
		const char * res_nm_165;
		const char * res_nm_166;
		const char * res_nm_167;
		const char * res_nm_168;
		const char * res_nm_169;
		const char * res_nm_170;
		const char * res_nm_171;
		const char * res_nm_172;
		const char * res_nm_173;
		const char * res_nm_174;
		const char * res_nm_175;
		const char * res_nm_176;
		const char * res_nm_177;
		const char * res_nm_178;
		const char * res_nm_179;
		const char * res_nm_180;
		const char * res_nm_181;
		const char * res_nm_182;
		const char * res_nm_183;
		const char * res_nm_184;
		const char * res_nm_185;
		const char * res_nm_186;
		const char * res_nm_187;
		const char * res_nm_188;
		const char * res_nm_189;
		const char * res_nm_190;
		const char * res_nm_191;
		const char * res_nm_192;
		const char * res_nm_193;
		const char * res_nm_194;
		const char * res_nm_195;
		const char * res_nm_196;
		const char * res_nm_197;
		const char * res_nm_198;
		const char * res_nm_199;
		const char * res_nm_200;
		const char * res_nm_201;
		const char * res_nm_202;
		const char * res_nm_203;
		const char * res_nm_204;
		const char * res_nm_205;
		const char * res_nm_206;
		const char * res_nm_207;
		const char * res_nm_208;
		const char * res_nm_209;
		const char * res_nm_210;
		const char * res_nm_211;
		const char * res_nm_212;
		const char * res_nm_213;
		const char * res_nm_214;
		const char * res_nm_215;
		const char * res_nm_216;
		const char * res_nm_217;
		const char * res_nm_218;
		const char * res_nm_219;
		const char * res_nm_220;
		const char * res_nm_221;
		const char * res_nm_222;
		const char * res_nm_223;
		const char * res_nm_224;
		const char * res_nm_225;
		const char * res_nm_226;
		const char * res_nm_227;
		const char * res_nm_228;
		const char * res_nm_229;
		const char * res_nm_230;
		const char * res_nm_231;
		const char * res_nm_232;
		const char * res_nm_233;
		const char * res_nm_234;
		const char * res_nm_235;
		const char * res_nm_236;
		const char * res_nm_237;
		const char * res_nm_238;
		const char * res_nm_239;
		const char * res_nm_240;
		const char * res_nm_241;
		const char * res_nm_242;
		const char * res_nm_243;
		const char * res_nm_244;
		const char * res_nm_245;
		const char * res_nm_246;
		const char * res_nm_247;
		const char * res_nm_248;
		const char * res_nm_249;
		const char * res_nm_250;
		const char * res_nm_251;
		const char * res_nm_252;
		const char * res_nm_253;
		const char * res_nm_254;
		const char * res_nm_255;
	};
} RESOURCE_NAME_LIST;

// for RC side
#define RES_SOLVE(arg_typ_idx,arg_val_ptr)	\
	(RTYP_IDX_CHARSTR==arg_typ_idx || RTYP_IDX_FORMSTR==arg_typ_idx || RTYP_IDX_DEVCTRL==arg_typ_idx)?((char *)arg_val_ptr):( \
		(RTYP_IDX_UINT8==arg_typ_idx)?(*(uint8_t *)arg_val_ptr):( \
			(RTYP_IDX_INT8==arg_typ_idx)?(*(int8_t *)arg_val_ptr):( \
				(RTYP_IDX_UINT16==arg_typ_idx)?(*(uint16_t *)arg_val_ptr):( \
					(RTYP_IDX_INT16==arg_typ_idx)?(*(int16_t *)arg_val_ptr):( \
						(RTYP_IDX_UINT32==arg_typ_idx)?(*(uint32_t *)arg_val_ptr):( \
							(RTYP_IDX_INT32==arg_typ_idx)?(*(int32_t *)arg_val_ptr):0 ))))))

#endif // UI_RESOURCE_CODE_H__