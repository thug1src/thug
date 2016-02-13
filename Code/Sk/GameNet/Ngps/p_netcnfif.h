/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5
 */
/* 
 *      Netcnf Interface Library
 *
 *                         Version 1.2
 *                         Shift-JIS
 *
 *      Copyright (C) 2002 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                         netcnfif.h
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.1            2002.01.28      tetsu       First version
 *       1.2            2002.02.10      tetsu       Add SCE_NETCNFIF_CHECK_ADDITIONAL_AT
 *                                                  Add sceNETCNFIF_TOO_LONG_STR
 */

#ifndef __netcnfif_common_h_
#define __netcnfif_common_h_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Sifrpc 用 */
#define SCE_NETCNFIF_SSIZE     (4096)       /* 送受信するデータバッファのサイズ */
#define SCE_NETCNFIF_INTERFACE (0x80001101) /* リクエスト識別子 */

/* Sifrpc サービス関数用番号 */
#define SCE_NETCNFIF_GET_COUNT           (0)   /* ネットワーク設定ファイルの数を取得 */
#define SCE_NETCNFIF_GET_LIST            (1)   /* ネットワーク設定ファイルのリストを取得 */
#define SCE_NETCNFIF_LOAD_ENTRY          (2)   /* ネットワーク設定ファイルの内容を取得 */
#define SCE_NETCNFIF_ADD_ENTRY           (3)   /* ネットワーク設定ファイルの追加 */
#define SCE_NETCNFIF_EDIT_ENTRY          (4)   /* ネットワーク設定ファイルの編集 */
#define SCE_NETCNFIF_DELETE_ENTRY        (5)   /* ネットワーク設定ファイルの削除 */
#define SCE_NETCNFIF_SET_LATEST_ENTRY    (6)   /* ネットワーク設定ファイルのリストを編集 */
#define SCE_NETCNFIF_DELETE_ALL          (7)   /* あなたのネットワーク設定ファイルを削除 */
#define SCE_NETCNFIF_CHECK_CAPACITY      (8)   /* デバイスの残り容量をチェック */
#define SCE_NETCNFIF_CHECK_ADDITIONAL_AT (9)   /* 追加 AT コマンドをチェック */
#define SCE_NETCNFIF_GET_ADDR            (100) /* IOP 側の受信領域(sceNetcnfifData)のアドレスを取得 */
#define SCE_NETCNFIF_ALLOC_WORKAREA      (101) /* IOP 側のワークエリアを確保 */
#define SCE_NETCNFIF_FREE_WORKAREA       (102) /* IOP 側のワークエリアを解放 */
#define SCE_NETCNFIF_SET_ENV             (103) /* IOP 側の sceNetCnfEnv 領域に sceNetcnfifData の内容を設定 */

/* エラーコード(-18 までは netcnf.irx と同等) */
#define sceNETCNFIF_NG               (-1)   /* その他のエラー */
#define sceNETCNFIF_ALLOC_ERROR      (-2)   /* メモリの確保に失敗 */
#define sceNETCNFIF_OPEN_ERROR       (-3)   /* ファイルを開けない */
#define sceNETCNFIF_READ_ERROR       (-4)   /* 読み込みに失敗 */
#define sceNETCNFIF_WRITE_ERROR      (-5)   /* 書き込みに失敗 */
#define sceNETCNFIF_SEEK_ERROR       (-6)   /* ファイルサイズ取得に失敗 */
#define sceNETCNFIF_REMOVE_ERROR     (-7)   /* 削除に失敗 */
#define sceNETCNFIF_ENTRY_NOT_FOUND  (-8)   /* 設定がない */
#define sceNETCNFIF_INVALID_FNAME    (-9)   /* 設定管理ファイルのパス名が不正 */
#define sceNETCNFIF_INVALID_TYPE     (-10)  /* あなたのネットワーク設定ファイルの種類が不正 */
#define sceNETCNFIF_INVALID_USR_NAME (-11)  /* 設定名が不正 */
#define sceNETCNFIF_TOO_MANY_ENTRIES (-12)  /* 設定数が最大数に達している */
#define sceNETCNFIF_ID_ERROR         (-13)  /* ID が取得できない */
#define sceNETCNFIF_SYNTAX_ERROR     (-14)  /* 設定内容が不正 */
#define sceNETCNFIF_MAGIC_ERROR      (-15)  /* 他の "PlayStation 2" で保存された設定 */
#define sceNETCNFIF_CAPACITY_ERROR   (-16)  /* デバイスの残り容量が足りない */
#define sceNETCNFIF_UNKNOWN_DEVICE   (-17)  /* 未知のデバイスが指定されている */
#define sceNETCNFIF_IO_ERROR         (-18)  /* IO エラー */
#define sceNETCNFIF_TOO_LONG_STR     (-19)  /* 指定された文字列が長すぎる */
#define sceNETCNFIF_NO_DATA          (-100) /* データが設定されてない */

/* Netcnf Interface に必要なデータ */
typedef struct sceNetcnfifArg{
    int data;               /* その他の引数/その他の返り値 */
    int f_no_decode;        /* no_decode フラグ */
    int type;               /* あなたのネットワーク設定ファイルの種類 */
    int addr;               /* EE 側の受信領域のアドレス/IOP 側アドレスの返り値 */
    char fname[256];        /* 設定管理ファイルのパス名/追加 AT コマンド */
    char usr_name[256];     /* 設定名 */
    char new_usr_name[256]; /* 新しい設定名 */
} sceNetcnfifArg_t;

enum
{
    sceNetcnfifArg_f_no_decode_off, /* f_no_decode を設定しない */
    sceNetcnfifArg_f_no_decode_on   /* f_no_decode を設定する */
};

enum
{
    sceNetcnfifArg_type_net, /* 組み合わせ */
    sceNetcnfifArg_type_ifc, /* 接続プロバイダ設定 */
    sceNetcnfifArg_type_dev  /* 接続機器設定 */
};

/* あなたのネットワーク設定ファイルのリスト */
typedef struct sceNetcnfifList{
    int type;           /* あなたのネットワーク設定ファイルの種類 */
    int stat;           /* ファイルステータス */
    char sys_name[256]; /* ファイル名 */
    char usr_name[256]; /* 設定名 */
    int p0[14];         /* 予約領域0 */
} sceNetcnfifList_t __attribute__((aligned(64)));

/* あなたのネットワーク設定ファイルに保存されるデータ */
typedef struct sceNetcnfifData{
    char attach_ifc[256];      /* 接続プロバイダ設定ファイル名(net) */
    char attach_dev[256];      /* 接続機器設定ファイル名(net) */
    char dhcp_host_name[256];  /* DHCP用ホスト名(ifc) */
    char address[256];         /* IPアドレス(ifc) */
    char netmask[256];         /* ネットマスク(ifc) */
    char gateway[256];         /* デフォルトルータ(ifc) */
    char dns1_address[256];    /* プライマリDNS(ifc) */
    char dns2_address[256];    /* セカンダリDNS(ifc) */
    char phone_numbers1[256];  /* 接続先電話番号1(ifc) */
    char phone_numbers2[256];  /* 接続先電話番号2(ifc) */
    char phone_numbers3[256];  /* 接続先電話番号3(ifc) */
    char auth_name[256];       /* ユーザID(ifc) */
    char auth_key[256];        /* パスワード(ifc) */
    char peer_name[256];       /* 接続先の認証名(ifc) */
    char vendor[256];          /* ベンダ名(dev) */
    char product[256];         /* プロダクト名(dev) */
    char chat_additional[256]; /* 追加ATコマンド(dev) */
    char outside_number[256];  /* 外線発信番号設定(番号設定部分)(dev) */
    char outside_delay[256];   /* 外線発信番号設定(遅延設定部分)(dev) */
    int ifc_type;              /* デバイスレイヤの種別(ifc) */
    int mtu;                   /* MTUの設定(ifc) */
    int ifc_idle_timeout;      /* 回線切断設定(ifc) */
    int dev_type;              /* デバイスレイヤの種別(dev) */
    int phy_config;            /* イーサネット接続機器の動作モード(dev) */
    int dialing_type;          /* ダイアル方法(dev) */
    int dev_idle_timeout;      /* 回線切断設定(dev) */
    int p0;                    /* 予約領域0 */
    unsigned char dhcp;        /* DHCP使用・不使用(ifc) */
    unsigned char dns1_nego;   /* DNSサーバアドレスを自動取得する・しない(ifc) */
    unsigned char dns2_nego;   /* DNSサーバアドレスを自動取得する・しない(ifc) */
    unsigned char f_auth;      /* 認証方法の指定有り(ifc) */
    unsigned char auth;        /* 認証方法(ifc) */
    unsigned char pppoe;       /* PPPoE使用・不使用(ifc) */
    unsigned char prc_nego;    /* PFCネゴシエーションの禁止(ifc) */
    unsigned char acc_nego;    /* ACFCネゴシエーションの禁止(ifc) */
    unsigned char accm_nego;   /* ACCMネゴシエーションの禁止(ifc) */
    unsigned char p1;          /* 予約領域1 */
    unsigned char p2;          /* 予約領域2 */
    unsigned char p3;          /* 予約領域3 */
    int p4[5];                 /* 予約領域4 */
} sceNetcnfifData_t __attribute__((aligned(64)));

enum
{
    sceNetcnfifData_type_no_set = -1, /* 設定しない */
    sceNetcnfifData_type_eth = 1,     /* type eth */
    sceNetcnfifData_type_ppp,         /* type ppp */
    sceNetcnfifData_type_nic          /* type nic */
};

enum
{
    sceNetcnfifData_mtu_no_set = -1,   /* 設定しない */
    sceNetcnfifData_mtu_default = 1454 /* mtu 1454 */
};

enum
{
    sceNetcnfifData_idle_timeout_no_set = -1 /* 設定しない */
};

enum
{
    sceNetcnfifData_phy_config_no_set = -1, /* 設定しない */
    sceNetcnfifData_phy_config_auto = 1,    /* phy_config auto */
    sceNetcnfifData_phy_config_10,          /* phy_config 10 */
    sceNetcnfifData_phy_config_10_FD,       /* phy_config 10_fd */
    sceNetcnfifData_phy_config_TX = 5,      /* phy_config tx */
    sceNetcnfifData_phy_config_TX_FD        /* phy_config tx_fd */
};

enum
{
    sceNetcnfifData_dialing_type_no_set = -1, /* 設定しない */
    sceNetcnfifData_dialing_type_tone = 0,    /* dialing_type tone */
    sceNetcnfifData_dialing_type_pulse        /* dialing_type pulse */
};

enum
{
    sceNetcnfifData_dhcp_no_set = 0xff, /* 設定しない */
    sceNetcnfifData_dhcp_no_use = 0,    /* -dhcp */
    sceNetcnfifData_dhcp_use            /* dhcp */
};

enum
{
    sceNetcnfifData_dns_nego_no_set = 0xff, /* 設定しない */
    sceNetcnfifData_dns_nego_on = 1         /* want.dns1_nego/want.dns2_nego */
};

enum
{
    sceNetcnfifData_f_auth_off, /* allow.auth chap/pap を設定しない */
    sceNetcnfifData_f_auth_on   /* allow.auth chap/pap を設定する */
};

enum
{
    sceNetcnfifData_auth_chap_pap = 4 /* allow.auth chap/pap */
};

enum
{
    sceNetcnfifData_pppoe_no_set = 0xff, /* 設定しない */
    sceNetcnfifData_pppoe_use = 1        /* pppoe */
};

enum
{
    sceNetcnfifData_prc_nego_no_set = 0xff, /* 設定しない */
    sceNetcnfifData_prc_nego_off = 0        /* -want.prc_nego */
};

enum
{
    sceNetcnfifData_acc_nego_no_set = 0xff, /* 設定しない */
    sceNetcnfifData_acc_nego_off = 0        /* -want.acc_nego */
};

enum
{
    sceNetcnfifData_accm_nego_no_set = 0xff, /* 設定しない */
    sceNetcnfifData_accm_nego_off = 0        /* -want.accm_nego */
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__netcnfif_common_h_ */
