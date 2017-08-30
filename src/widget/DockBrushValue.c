/*****************************************
 * DockBrushValue
 *
 * ブラシの項目値
 *****************************************/
/*
 * DockBrushValue (mContainer : 垂直)
 *  |- 名前、ボタン
 *  |- mContainerView (area = 垂直コンテナ)
 *      |- mExpander
 *          |- 項目
 *      |- ...
 *
 */

#include "mDef.h"
#include "mWidget.h"
#include "mContainerDef.h"
#include "mContainer.h"
#include "mContainerView.h"
#include "mLabel.h"
#include "mButton.h"
#include "mCheckButton.h"
#include "mArrowButton.h"
#include "mBitImgButton.h"
#include "mComboBox.h"
#include "mExpander.h"
#include "mEvent.h"
#include "mMenu.h"
#include "mStr.h"
#include "mUtilStr.h"
#include "mTrans.h"

#include "defConfig.h"
#include "defPixelMode.h"

#include "ValueBar.h"
#include "SelMaterial.h"
#include "PressureWidget.h"

#include "BrushList.h"
#include "BrushItem.h"
#include "ConfigData.h"

#include "trgroup.h"


//-----------------------

typedef struct _DockBrushValue
{
	mWidget wg;
	mContainerData ct;

	mContainerView *ctview;

	mWidget *ctview_area,	//mContainerView のエリア
		*wg_brushsave,
		*wg_watermenu,
		*wg_btt_waterpreset[CONFIG_WATER_PRESET_NUM];
	mLabel *label_name;
	mCheckButton *ck_travelling_dir,
		*ck_antialias,
		*ck_curve;
	mComboBox *cb_type,
		*cb_pixmode,
		*cb_hosei_type,
		*cb_hosei_str,
		*cb_press_type;
	ValueBar *bar_radius,
		*bar_opacity,
		*bar_min_size,
		*bar_min_opacity,
		*bar_interval,
		*bar_rand_size,
		*bar_rand_pos,
		*bar_water[3],
		*bar_hard,
		*bar_rough,
		*bar_angle,
		*bar_angle_rand;
	SelMaterial *selmat_brush,
		*selmat_texture;
	PressureWidget *pressure_wg;
}DockBrushValue;

//-----------------------

//ウィジェットID と共通
enum
{
	TRID_HEADER_MAIN = 10,
	TRID_HEADER_MIN,
	TRID_HEADER_RANDOM,
	TRID_HEADER_WATER,
	TRID_HEADER_SHAPE,
	TRID_HEADER_SHAPE_ROTATE,
	TRID_HEADER_PRESSURE,
	TRID_HEADER_OTHER,

	TRID_TYPE = 100,
	TRID_RADIUS,
	TRID_OPACITY,
	TRID_PIXELMODE,
	TRID_HOSEI_TYPE,
	TRID_MIN_SIZE,
	TRID_MIN_OPACITY,
	TRID_INTERVAL,
	TRID_RAND_SIZE,
	TRID_RAND_POS,
	TRID_WATER_PRESET,
	TRID_WATER_PARAM1,
	TRID_WATER_PARAM2,
	TRID_WATER_PARAM3,
	TRID_HARD,
	TRID_ROUGH,
	TRID_TRAVELLING_DIR,
	TRID_ANGLE,
	TRID_ANGLE_RAND,
	TRID_TEXTURE,
	TRID_ANTIALIAS,
	TRID_CURVE,

	TRID_TYPE_TOP = 200,
	TRID_HOSEITYPE_TOP = 220,
	TRID_PRESSURETYPE_TOP = 230,

	TRID_WATERPRESET_REGIST = 240,
	TRID_WATERPRESET_RESET
};

//文字列IDと共通しないウィジェットID
enum
{
	WID_BTT_SAVE_BRUSH = 2000,
	WID_BTT_DETAIL_OPTION,
	WID_CB_HOSEI_STR,
	WID_BTT_WATER_PRESET_MENU,
	WID_SELMAT_BRUSH,
	WID_CB_PRESSURE_TYPE,
	WID_PRESSURE_EDIT,
	
	WID_BTT_WATER_PRESET_TOP = 2100
};

//-----------------------
/* ConfigData::dockbrush_expand_flags
 * 各項目の展開フラグ */

enum
{
	EXPAND_F_MAIN   = 1<<0,
	EXPAND_F_MIN    = 1<<1,
	EXPAND_F_RANDOM = 1<<2,
	EXPAND_F_WATER  = 1<<3,
	EXPAND_F_SHAPE  = 1<<4,
	EXPAND_F_SHAPE_ROTATE = 1<<5,
	EXPAND_F_PRESSURE = 1<<6,
	EXPAND_F_OTHER  = 1<<7
};

//-----------------------

mBool BrushDetailDlg_run(mWindow *owner,BrushItem *item);

static void _create_widget(DockBrushValue *p,mWidget *parent);
static int _event_handle(mWidget *wg,mEvent *ev);

//-----------------------

//保存ボタン (1bit:12x12)
static const uint8_t g_img1bit_save[] =
{
	0xff,0x0f,0x0f,0x0f,0xff,0x0f,0xff,0x0f,0xff,0x0f,0xff,0x0f,0x07,0x0e,0x07,0x0e,
	0x07,0x0e,0x07,0x0e,0x07,0x0e,0xff,0x0f
};

//詳細設定ボタン (2bit:12x12)
static const unsigned char g_img2bit_option[]={
0x55,0x55,0x05,0xa9,0xaa,0x06,0x59,0x55,0x06,0xa9,0xaa,0x06,0x59,0x55,0x06,0xa9,
0xaa,0x29,0x59,0x59,0x95,0xa9,0x5a,0x96,0x59,0x96,0x5a,0xa9,0x5a,0x96,0x55,0x59,
0x95,0x00,0xa0,0x29 };

//-----------------------



//==============================
// sub
//==============================


/** 塗りのリストセット */

static void _set_pixelmode_list(DockBrushValue *p,BrushItem *pi)
{
	int num;

	num = BrushItem_getPixelModeNum(pi);

	//リスト変更

	if(num != mComboBoxGetItemNum(p->cb_pixmode))
	{
		M_TR_G(TRGROUP_PIXELMODE_BRUSH);

		mComboBoxDeleteAllItem(p->cb_pixmode);

		mComboBoxAddTrItems(p->cb_pixmode, num, 0, 0);
	}

	//選択

	mComboBoxSetSel_index(p->cb_pixmode, pi->pixelmode);
}

/** 半径のバー情報セット */

static void _set_radius_bar(DockBrushValue *p,BrushItem *pi)
{
	if(pi->type == BRUSHITEM_TYPE_FINGER)
	{
		ValueBar_setStatus_dig(p->bar_radius, 0,
			BRUSHITEM_FINGER_RADIUS_MIN, BRUSHITEM_FINGER_RADIUS_MAX, pi->radius);
	}
	else
		ValueBar_setStatus_dig(p->bar_radius, 1, pi->radius_min, pi->radius_max, pi->radius);
}

/** 形状画像による項目の有効/無効 */

static void _enable_by_shapeimage(DockBrushValue *p)
{
	BrushItem *pi = BrushList_getEditItem();

	//ドットペン/指先以外で、形状画像がない場合、「硬さ」を有効

	mWidgetEnable(M_WIDGET(p->bar_hard),
		(pi->type != BRUSHITEM_TYPE_DOTPEN && pi->type != BRUSHITEM_TYPE_FINGER && !pi->shape_path));
}

/** ブラシタイプ変更による有効/無効 */

static void _enable_by_brushtype(DockBrushValue *p,int type)
{
	mBool noblur,nodot,no_blur_dot,water,nofinger,no_dot_finger;
	int i;

	noblur = (type != BRUSHITEM_TYPE_BLUR);
	nodot  = (type != BRUSHITEM_TYPE_DOTPEN);
	nofinger = (type != BRUSHITEM_TYPE_FINGER);
	water = (type == BRUSHITEM_TYPE_WATER);
	
	no_blur_dot = (noblur && nodot);
	no_dot_finger = (nodot && nofinger);

	/* 水彩以外時は [水彩] を無効。
	 * 水彩時は、[塗り] を無効。
	 *
	 * ぼかし時は、[塗り][濃度最小][間隔] を無効。
	 * 
	 * ドットペン時は、[半径][最小&間隔][ランダムサイズ&位置]
	 *  [ブラシ形状][形状回転][筆圧][アンチエイリアス/曲線補間] を無効。
	 *
	 * 指先時は、[半径][濃度][補正][テクスチャ] 以外を無効。 */

	mWidgetEnable(M_WIDGET(p->bar_radius), nodot);
	mWidgetEnable(M_WIDGET(p->cb_pixmode), (noblur && !water && nofinger));

	mWidgetEnable(M_WIDGET(p->bar_min_size), no_dot_finger);
	mWidgetEnable(M_WIDGET(p->bar_min_opacity), (no_blur_dot && nofinger));
	mWidgetEnable(M_WIDGET(p->bar_interval), (no_blur_dot && nofinger));

	mWidgetEnable(M_WIDGET(p->bar_rand_size), no_dot_finger);
	mWidgetEnable(M_WIDGET(p->bar_rand_pos), no_dot_finger);

	//水彩

	for(i = 0; i < 3; i++)
		mWidgetEnable(M_WIDGET(p->bar_water[i]), water);

	for(i = 0; i < CONFIG_WATER_PRESET_NUM; i++)
		mWidgetEnable(p->wg_btt_waterpreset[i], water);

	mWidgetEnable(p->wg_watermenu, water);

	//

	mWidgetEnable(M_WIDGET(p->selmat_brush), no_dot_finger);
	mWidgetEnable(M_WIDGET(p->bar_rough), no_dot_finger);

	mWidgetEnable(M_WIDGET(p->ck_travelling_dir), no_dot_finger);
	mWidgetEnable(M_WIDGET(p->bar_angle), no_dot_finger);
	mWidgetEnable(M_WIDGET(p->bar_angle_rand), no_dot_finger);

	mWidgetEnable(M_WIDGET(p->cb_press_type), no_dot_finger);
	mWidgetEnable(M_WIDGET(p->pressure_wg), no_dot_finger);

	mWidgetEnable(M_WIDGET(p->ck_antialias), no_dot_finger);
	mWidgetEnable(M_WIDGET(p->ck_curve), no_dot_finger);

	_enable_by_shapeimage(p);
}


//==============================
// main
//==============================


/** DockBrushValue 作成 */

DockBrushValue *DockBrushValue_new(mWidget *parent)
{
	DockBrushValue *p;
	mWidget *ct;
	mBitImgButton *bib;
	mContainerView *ctv;

	//------ DockBrushValue (mContainer)

	p = (DockBrushValue *)mContainerNew(sizeof(DockBrushValue), parent);

	p->wg.notifyTarget = MWIDGET_NOTIFYTARGET_SELF;
	p->wg.fLayout = MLF_EXPAND_WH;
	p->wg.event = _event_handle;

	//------ ブラシ名＋ボタン

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 5, MLF_EXPAND_W);

	ct->margin.bottom = 4;

	//ブラシ名

	p->label_name = mLabelCreate(ct,  MLABEL_S_BORDER, MLF_EXPAND_W | MLF_MIDDLE, 0, NULL);

	//保存ボタン

	p->wg_brushsave = (mWidget *)mBitImgButtonCreate(ct, WID_BTT_SAVE_BRUSH, 0, MLF_MIDDLE, 0);

	mBitImgButtonSetImage(M_BITIMGBUTTON(p->wg_brushsave),
		MBITIMGBUTTON_TYPE_1BIT_BLACK_WHITE, g_img1bit_save, 12, 12);

	//詳細設定ボタン

	bib = mBitImgButtonCreate(ct, WID_BTT_DETAIL_OPTION, 0, MLF_MIDDLE, 0);

	mBitImgButtonSetImage(bib, MBITIMGBUTTON_TYPE_2BIT_TP_BLACK_WHITE, g_img2bit_option, 12, 12);

	//------ mContainerView (項目部分はスクロールできるように)

	p->ctview = ctv = mContainerViewNew(0, M_WIDGET(p), 0);

	ctv->wg.fLayout = MLF_EXPAND_WH;

	//内容のコンテナ

	ctv->cv.area = ct = mContainerCreate(M_WIDGET(ctv), MCONTAINER_TYPE_VERT, 0, 0, MLF_EXPAND_WH);

	p->ctview_area = ct;

	mContainerSetPadding_b4(M_CONTAINER(ct), M_MAKE_DW4(1,0,3,4));

	//各項目

	M_TR_G(TRGROUP_DOCK_BRUSH_VALUE);

	_create_widget(p, ct);
	
	return p;
}

/** (外部から) 半径の値をセット
 *
 * @param p  NULL でブラシデータの値のみ変更 */

void DockBrushValue_setRadius(DockBrushValue *p,int val)
{
	//指先の場合、/10

	if(BrushList_getEditItem()->type == BRUSHITEM_TYPE_FINGER)
		val /= 10;

	//[!] 範囲外の場合、値が調整されることがある
	
	val = BrushList_updateval_radius(val);

	if(p) ValueBar_setPos(p->bar_radius, val);
}

/** 項目値セット */

void DockBrushValue_setValue(DockBrushValue *p)
{
	BrushItem *pi = BrushList_getEditItem();
	mBool exist;
	int i;

	//選択アイテムがあるか

	exist = (BrushList_getSelItem() != NULL);

	//名前

	mLabelSetText(p->label_name, (exist)? pi->name: NULL);

	//保存ボタン

	mWidgetEnable(p->wg_brushsave, (exist && !(pi->flags & BRUSHITEM_F_AUTO_SAVE)));

	//------ メイン

	//タイプ

	mComboBoxSetSel_index(p->cb_type, pi->type);

	//半径

	_set_radius_bar(p, pi);

	//濃度

	ValueBar_setPos(p->bar_opacity, pi->opacity);

	//塗り

	_set_pixelmode_list(p, pi);

	//補正

	mComboBoxSetSel_index(p->cb_hosei_type, pi->hosei_type);
	mComboBoxSetSel_index(p->cb_hosei_str, pi->hosei_strong - 1);

	//最小、間隔

	ValueBar_setPos(p->bar_min_size, pi->min_size);
	ValueBar_setPos(p->bar_min_opacity, pi->min_opacity);
	ValueBar_setPos(p->bar_interval, pi->interval);

	//ランダム

	ValueBar_setPos(p->bar_rand_size, pi->rand_size_min);
	ValueBar_setPos(p->bar_rand_pos, pi->rand_pos_range);

	//水彩

	for(i = 0; i < 3; i++)
		ValueBar_setPos(p->bar_water[i], pi->water_param[i]);

	//形状

	SelMaterial_setName(p->selmat_brush, pi->shape_path);
	ValueBar_setPos(p->bar_hard, pi->hardness);
	ValueBar_setPos(p->bar_rough, pi->roughness);

	//回転

	mCheckButtonSetState(p->ck_travelling_dir, pi->flags & BRUSHITEM_F_ROTATE_TRAVELLING_DIR);
	ValueBar_setPos(p->bar_angle, pi->rotate_angle);
	ValueBar_setPos(p->bar_angle_rand, pi->rotate_rand_w);

	//筆圧

	mComboBoxSetSel_index(p->cb_press_type, pi->pressure_type);
	PressureWidget_setValue(p->pressure_wg, pi->pressure_type, pi->pressure_val);

	//ほか

	SelMaterial_setName(p->selmat_texture, pi->texture_path);
	mCheckButtonSetState(p->ck_antialias, pi->flags & BRUSHITEM_F_ANTIALIAS);
	mCheckButtonSetState(p->ck_curve, pi->flags & BRUSHITEM_F_CURVE);

	//----- 有効/無効

	_enable_by_brushtype(p, pi->type);
}


//==============================
// イベント
//==============================


/** 水彩プリセット呼び出し */

static void _waterpreset_call(DockBrushValue *p,int no)
{
	uint32_t v = APP_CONF->water_preset[no];
	int i,val;

	for(i = 0; i < 3; i++)
	{
		val = (v >> (i * 10)) & 1023;

		ValueBar_setPos(p->bar_water[i], val);

		BrushList_updateval_waterparam(i, val);
	}
}

/** 水彩プリセットメニュー実行 */

static void _runmenu_waterpreset(DockBrushValue *p)
{
	mMenu *menu;
	mStr str = MSTR_INIT;
	const char *regstr;
	mMenuItemInfo *mi;
	int i;
	mBox box;

	M_TR_G(TRGROUP_DOCK_BRUSH_VALUE);

	menu = mMenuNew();

	//* に登録

	regstr = M_TR_T(TRID_WATERPRESET_REGIST);

	for(i = 0; i < CONFIG_WATER_PRESET_NUM; i++)
	{
		mStrSetFormat(&str, regstr, 'A' + i);
		mMenuAddText_copy(menu, i, str.buf);
	}

	mStrFree(&str);

	//

	mMenuAddSep(menu);
	mMenuAddText_static(menu, 100, M_TR_T(TRID_WATERPRESET_RESET));

	//

	mWidgetGetRootBox(p->wg_watermenu, &box);

	mi = mMenuPopup(menu, NULL, box.x, box.y + box.h, 0);
	i = (mi)? mi->id: -1;

	mMenuDestroy(menu);

	//--------

	if(i == -1) return;

	if(i == 100)
		//初期値に戻す
		ConfigData_waterPreset_default();
	else
	{
		//登録

		APP_CONF->water_preset[i] = ValueBar_getPos(p->bar_water[0])
			| (ValueBar_getPos(p->bar_water[1]) << 10)
			| (ValueBar_getPos(p->bar_water[2]) << 20);
	}
}

/** 詳細設定 */

static void _detail_option(DockBrushValue *p)
{
	BrushItem *item = BrushList_getEditItem();

	if(BrushDetailDlg_run(p->wg.toplevel, item))
	{
		BrushList_updateval_detail();

		//保存ボタン

		mWidgetEnable(p->wg_brushsave,
			(BrushList_getSelItem() && !(item->flags & BRUSHITEM_F_AUTO_SAVE)));

		//半径バー

		_set_radius_bar(p, item);
	}
}

/** イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	DockBrushValue *p = (DockBrushValue *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		intptr_t p1,p2;
		int id,type;

		id = ev->notify.id;
		type = ev->notify.type;
		p1 = ev->notify.param1;
		p2 = ev->notify.param2;

		//水彩プリセット呼び出し

		if(id >= WID_BTT_WATER_PRESET_TOP && id < WID_BTT_WATER_PRESET_TOP + CONFIG_WATER_PRESET_NUM)
		{
			_waterpreset_call(p, id - WID_BTT_WATER_PRESET_TOP);
			return 1;
		}

		//
	
		switch(id)
		{
			//手動保存ボタン
			case WID_BTT_SAVE_BRUSH:
				BrushList_saveitem_manual();
				break;
			//詳細設定
			case WID_BTT_DETAIL_OPTION:
				_detail_option(p);
				break;

			//タイプ
			case TRID_TYPE:
				if(type == MCOMBOBOX_N_CHANGESEL)
				{
					BrushList_updateval_type(p2);

					_set_pixelmode_list(p, BrushList_getEditItem());
					_set_radius_bar(p, BrushList_getEditItem());
					_enable_by_brushtype(p, p2);
				}
				break;
			//半径
			case TRID_RADIUS:
				if(p2) //値の確定時
					BrushList_updateval_radius(p1);
				break;
			//濃度
			case TRID_OPACITY:
				if(p2) BrushList_updateval_opacity(p1);
				break;
			//塗りタイプ
			case TRID_PIXELMODE:
				if(type == MCOMBOBOX_N_CHANGESEL)
					BrushList_updateval_pixelmode(p2);
				break;
			//補正タイプ
			case TRID_HOSEI_TYPE:
				if(type == MCOMBOBOX_N_CHANGESEL)
					BrushList_updateval_hosei_type(p2);
				break;
			//補正強さ
			case WID_CB_HOSEI_STR:
				if(type == MCOMBOBOX_N_CHANGESEL)
					BrushList_updateval_hosei_str(p2);
				break;

			//サイズ最小
			case TRID_MIN_SIZE:
				if(p2) BrushList_updateval_min_size(p1);
				break;
			//濃度最小
			case TRID_MIN_OPACITY:
				if(p2) BrushList_updateval_min_opacity(p1);
				break;
			//間隔
			case TRID_INTERVAL:
				if(p2) BrushList_updateval_interval(p1);
				break;

			//ランダムサイズ
			case TRID_RAND_SIZE:
				if(p2) BrushList_updateval_random_size(p1);
				break;
			//ランダム位置
			case TRID_RAND_POS:
				if(p2) BrushList_updateval_random_pos(p1);
				break;

			//水彩パラメータ
			case TRID_WATER_PARAM1:
			case TRID_WATER_PARAM2:
			case TRID_WATER_PARAM3:
				if(p2)
					BrushList_updateval_waterparam(id - TRID_WATER_PARAM1, p1);
				break;
			//水彩プリセットメニュー
			case WID_BTT_WATER_PRESET_MENU:
				_runmenu_waterpreset(p);
				break;

			//形状画像
			case WID_SELMAT_BRUSH:
				BrushList_updateval_shape_path((const char *)p1);

				_enable_by_shapeimage(p);
				break;
			//硬さ
			case TRID_HARD:
				if(p2) BrushList_updateval_shape_hard(p1);
				break;
			//荒さ
			case TRID_ROUGH:
				if(p2) BrushList_updateval_shape_rough(p1);
				break;

			//進行方向
			case TRID_TRAVELLING_DIR:
				BrushList_updateval_travelling_dir();
				break;
			//角度
			case TRID_ANGLE:
				if(p2) BrushList_updateval_angle(p1);
				break;
			//角度ランダム
			case TRID_ANGLE_RAND:
				if(p2) BrushList_updateval_angle_radom(p1);
				break;

			//筆圧補正タイプ
			case WID_CB_PRESSURE_TYPE:
				if(type == MCOMBOBOX_N_CHANGESEL)
				{
					BrushList_updateval_pressure_type(p2);

					PressureWidget_setValue(p->pressure_wg,
						BrushList_getEditItem()->pressure_type,
						BrushList_getEditItem()->pressure_val);
				}
				break;
			//筆圧補正編集
			case WID_PRESSURE_EDIT:
				BrushList_updateval_pressure_value(
					PressureWidget_getValue(p->pressure_wg));
				break;

			//テクスチャ画像
			case TRID_TEXTURE:
				BrushList_updateval_texture_path((const char *)p1);
				break;
			//アンチエイリアス
			case TRID_ANTIALIAS:
				BrushList_updateval_antialias();
				break;
			//曲線補間
			case TRID_CURVE:
				BrushList_updateval_curve();
				break;

			//mExpander 展開状態変化
			case TRID_HEADER_MAIN:
			case TRID_HEADER_MIN:
			case TRID_HEADER_RANDOM:
			case TRID_HEADER_WATER:
			case TRID_HEADER_SHAPE:
			case TRID_HEADER_SHAPE_ROTATE:
			case TRID_HEADER_PRESSURE:
			case TRID_HEADER_OTHER:
				APP_CONF->dockbrush_expand_flags ^= ev->notify.widgetFrom->param;
			
				mWidgetReLayout(p->ctview_area);
				mContainerViewConstruct(p->ctview);
				break;
		}
	}

	return 1;
}


//==============================
// 項目ウィジェット作成
//==============================
/*
 * mExpander の param には、展開の ON/OFF 時用に EXPAND_F_* の値を入れておく。
 */


//----------- sub

/** mExpander 作成 */

static mExpander *_create_expander(mWidget *parent,int id,int flag)
{
	mExpander *p;

	p = mExpanderCreate(parent, id, MEXPANDER_S_HEADER_DARK,
			MLF_EXPAND_W, M_MAKE_DW4(0,5,0,0), M_TR_T(id));

	p->wg.param = flag;

	mExpanderSetPadding_b4(p, M_MAKE_DW4(0,5,0,2));

	mExpanderSetExpand(p, APP_CONF->dockbrush_expand_flags & flag);

	return p;
}

/** ラベル+バー作成 (縦) */

static ValueBar *_create_label_bar_vert(mExpander *ex,int id,int dig,int min,int max)
{
	mLabelCreate(M_WIDGET(ex), 0, 0, 0, M_TR_T(id));
	
	return ValueBar_new(M_WIDGET(ex), id, MLF_EXPAND_W, dig, min, max, min);
}

/** ラベル+バー作成 (横) */

static ValueBar *_create_label_bar_horz(mWidget *parent,int id,int dig,int min,int max)
{
	mLabelCreate(parent, 0, MLF_MIDDLE, 0, M_TR_T(id));
	
	return ValueBar_new(parent, id, MLF_EXPAND_W | MLF_MIDDLE, dig, min, max, min);
}

/** ラベル+コンボボックス (横) 作成
 *
 * @param groupid 負の値でグループはそのまま、先頭の文字列ID */

static mComboBox *_create_label_combo_horz(mWidget *parent,int id,int groupid,int itemnum)
{
	mComboBox *cb;

	mLabelCreate(parent, 0, MLF_MIDDLE, 0, M_TR_T(id));

	cb = mComboBoxCreate(parent, id, 0, MLF_EXPAND_W, 0);

	if(groupid < 0)
		mComboBoxAddTrItems(cb, itemnum, -groupid, 0);
	else
	{
		M_TR_G(groupid);
		mComboBoxAddTrItems(cb, itemnum, 0, 0);
		M_TR_G(TRGROUP_DOCK_BRUSH_VALUE);
	}

	return cb;
}


//------------ 各内容

/** メイン項目 */

static void _create_widget_main(DockBrushValue *p,mWidget *ct_top)
{
	mExpander *pex;
	mWidget *ct;
	int i;
	char m[8];

	//mExpander

	pex = _create_expander(ct_top, TRID_HEADER_MAIN, EXPAND_F_MAIN);

	pex->wg.margin.top = 0;

	mContainerSetTypeGrid(M_CONTAINER(pex), 2, 4, 4);

	//タイプ

	p->cb_type = _create_label_combo_horz(M_WIDGET(pex), TRID_TYPE, -TRID_TYPE_TOP, BRUSHITEM_TYPE_NUM);

	//半径

	p->bar_radius = _create_label_bar_horz(M_WIDGET(pex), TRID_RADIUS,
		1, BRUSHITEM_RADIUS_MIN, BRUSHITEM_RADIUS_MAX);

	//濃度

	p->bar_opacity = _create_label_bar_horz(M_WIDGET(pex), TRID_OPACITY, 0, 1, 100);

	//塗り

	p->cb_pixmode = _create_label_combo_horz(M_WIDGET(pex), TRID_PIXELMODE, TRGROUP_PIXELMODE_BRUSH, PIXELMODE_BRUSH_NUM);

	//補正

	mLabelCreate(M_WIDGET(pex), 0, MLF_MIDDLE, 0, M_TR_T(TRID_HOSEI_TYPE));

	ct = mContainerCreate(M_WIDGET(pex), MCONTAINER_TYPE_HORZ, 0, 4, MLF_EXPAND_W);

	p->cb_hosei_type = mComboBoxCreate(ct, TRID_HOSEI_TYPE, 0, 0, 0);
	p->cb_hosei_str = mComboBoxCreate(ct, WID_CB_HOSEI_STR, 0, MLF_EXPAND_W, 0);

	mComboBoxAddTrItems(p->cb_hosei_type, 4, TRID_HOSEITYPE_TOP, 0);
	mComboBoxSetWidthAuto(p->cb_hosei_type);

	for(i = 1; i <= BRUSHITEM_HOSEI_STRONG_MAX; i++)
	{
		mIntToStr(m, i);
		mComboBoxAddItem(p->cb_hosei_str, m, i);
	}
}

/** 最小&間隔項目 */

static void _create_widget_min(DockBrushValue *p,mWidget *ct_top)
{
	mExpander *pex;

	//mExpander

	pex = _create_expander(ct_top, TRID_HEADER_MIN, EXPAND_F_MIN);

	pex->ct.sepW = 2;

	//サイズ最小

	p->bar_min_size = _create_label_bar_vert(pex, TRID_MIN_SIZE, 1, 0, 1000);

	//濃度最小

	p->bar_min_opacity = _create_label_bar_vert(pex, TRID_MIN_OPACITY, 1, 0, 1000);

	//間隔

	p->bar_interval = _create_label_bar_vert(pex, TRID_INTERVAL, 2,
		BRUSHITEM_INTERVAL_MIN, BRUSHITEM_INTERVAL_MAX);
}

/** ランダム項目 */

static void _create_widget_random(DockBrushValue *p,mWidget *ct_top)
{
	mExpander *pex;

	//mExpander

	pex = _create_expander(ct_top, TRID_HEADER_RANDOM, EXPAND_F_RANDOM);

	pex->ct.sepW = 2;

	//サイズ最小

	p->bar_rand_size = _create_label_bar_vert(pex, TRID_RAND_SIZE, 1, 0, 1000);

	//位置

	p->bar_rand_pos = _create_label_bar_vert(pex, TRID_RAND_POS, 2, 0, BRUSHITEM_RANDOM_POS_MAX);
}

/** 水彩項目 */

static void _create_widget_water(DockBrushValue *p,mWidget *ct_top)
{
	mExpander *pex;
	mWidget *ct;
	int i;
	char m[2] = {0,0};

	//mExpander

	pex = _create_expander(ct_top, TRID_HEADER_WATER, EXPAND_F_WATER);

	pex->ct.sepW = 2;

	//プリセット

	ct = mContainerCreate(M_WIDGET(pex), MCONTAINER_TYPE_HORZ, 0, 0, 0);

	mLabelCreate(ct, 0, MLF_MIDDLE, M_MAKE_DW4(0,0,5,0), M_TR_T(TRID_WATER_PRESET));

	for(i = 0; i < CONFIG_WATER_PRESET_NUM; i++)
	{
		m[0] = 'A' + i;
	
		p->wg_btt_waterpreset[i] = (mWidget *)mButtonCreate(ct, WID_BTT_WATER_PRESET_TOP + i,
			MBUTTON_S_REAL_WH, MLF_MIDDLE, 0, m);
	}

	p->wg_watermenu = (mWidget *)mArrowButtonCreate(ct, WID_BTT_WATER_PRESET_MENU,
		MARROWBUTTON_S_DOWN, MLF_MIDDLE, M_MAKE_DW4(8,0,0,0));

	//バー

	for(i = 0; i < 3; i++)
		p->bar_water[i] = _create_label_bar_vert(pex, TRID_WATER_PARAM1 + i, 1, 0, 1000);
}

/** ブラシ形状項目 */

static void _create_widget_shape(DockBrushValue *p,mWidget *ct_top)
{
	mExpander *pex;
	mWidget *ct;

	//mExpander

	pex = _create_expander(ct_top, TRID_HEADER_SHAPE, EXPAND_F_SHAPE);

	pex->ct.sepW = 5;

	//画像選択

	p->selmat_brush = SelMaterial_new(M_WIDGET(pex), WID_SELMAT_BRUSH, SELMATERIAL_TYPE_BRUSH_SHAPE);

	//-------

	ct = mContainerCreateGrid(M_WIDGET(pex), 2, 4, 6, MLF_EXPAND_W);

	//硬さ

	p->bar_hard = _create_label_bar_horz(ct, TRID_HARD, 0, 0, 100);

	//荒さ

	p->bar_rough = _create_label_bar_horz(ct, TRID_ROUGH, 0, 0, 100);
}

/** 形状回転項目 */

static void _create_widget_shape_rotate(DockBrushValue *p,mWidget *ct_top)
{
	mExpander *pex;

	//mExpander

	pex = _create_expander(ct_top, TRID_HEADER_SHAPE_ROTATE, EXPAND_F_SHAPE_ROTATE);
	pex->ct.sepW = 2;

	//進行方向

	p->ck_travelling_dir = mCheckButtonCreate(M_WIDGET(pex), TRID_TRAVELLING_DIR,
		0, 0, 0, M_TR_T(TRID_TRAVELLING_DIR), FALSE);

	//角度

	p->bar_angle = _create_label_bar_vert(pex, TRID_ANGLE, 0, 0, 359);

	//角度ランダム

	p->bar_angle_rand = _create_label_bar_vert(pex, TRID_ANGLE_RAND, 0, 0, 180);
}

/** 筆圧項目 */

static void _create_widget_pressure(DockBrushValue *p,mWidget *ct_top)
{
	mExpander *pex;

	//mExpander

	pex = _create_expander(ct_top, TRID_HEADER_PRESSURE, EXPAND_F_PRESSURE);
	pex->ct.sepW = 6;

	//タイプ

	p->cb_press_type = mComboBoxCreate(M_WIDGET(pex), WID_CB_PRESSURE_TYPE, 0, MLF_EXPAND_W, 0);

	mComboBoxAddTrItems(p->cb_press_type, 3, TRID_PRESSURETYPE_TOP, 0);

	//編集ウィジェット

	p->pressure_wg = PressureWidget_new(M_WIDGET(pex), WID_PRESSURE_EDIT);
}

/** ほか項目 */

static void _create_widget_other(DockBrushValue *p,mWidget *ct_top)
{
	mExpander *pex;

	//mExpander

	pex = _create_expander(ct_top, TRID_HEADER_OTHER, EXPAND_F_OTHER);
	pex->ct.sepW = 3;

	//テクスチャ

	mLabelCreate(M_WIDGET(pex), 0, 0, 0, M_TR_T(TRID_TEXTURE));

	p->selmat_texture = SelMaterial_new(M_WIDGET(pex), TRID_TEXTURE, SELMATERIAL_TYPE_BRUSH_TEXTURE);

	//アンチエイリアス

	p->ck_antialias = mCheckButtonCreate(M_WIDGET(pex), TRID_ANTIALIAS,
		0, 0, 0, M_TR_T(TRID_ANTIALIAS), FALSE);

	//曲線補間

	p->ck_curve = mCheckButtonCreate(M_WIDGET(pex), TRID_CURVE,
		0, 0, 0, M_TR_T(TRID_CURVE), FALSE);
}

//------------ main

/** ウィジェット作成メイン */

void _create_widget(DockBrushValue *p,mWidget *parent)
{
	_create_widget_main(p, parent);
	_create_widget_min(p, parent);
	_create_widget_random(p, parent);
	_create_widget_water(p, parent);
	_create_widget_shape(p, parent);
	_create_widget_shape_rotate(p, parent);
	_create_widget_pressure(p, parent);
	_create_widget_other(p, parent);
}

