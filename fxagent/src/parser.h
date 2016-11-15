
#ifndef _PARSER_H_
#define _PARSER_H_

#ifdef __cplusplus
extern "C"
{
#endif


/* type value */
#define CMD_OPKG_UPDATE "opkg"
#define CMD_RSP_REGISTER "rsp_gateway_online"
#define CMD_INSTALL_IPK "install"
#define CMD_UNINSTALL_IPK "uninstall"
#define CMD_UPGRADE_IPK "upgrade"
#define CMD_GET_INSTALL_IPK "query_installed"
#define CMD_GET_RUNNING_IPK "query_running"
#define CMD_PLUGIN_ACTION "plugin_action"

#define CMD_START_IPK_SERVICE "start" 
#define CMD_STOP_IPK_SERVICE "stop"
#define CMD_ENABLE_IPK_SERVICE "enable"
#define CMD_DISABLE_IPK_SERVICE "disable"

#define CMD_ALLJOYN_GET_OPERATION "get"
#define CMD_ALLJOYN_SET_OPERATION "set"

#define CMD_ALLJOYN_UPDATE_DEVICEID "rsp_device_online"

#define MAX_DEVICETYPE_LEN 32
#define MAX_DEVICEID_LEN 32
#define MAX_DEVICESN_LEN 32
#define MAX_USERID_LEN 32
#define MAX_VERSION_LEN 32
#define MAX_PLUGIN_URL_LIST_LEN 256

#define NOT_EMPTY 0
#define EMPTY -1
#define MAX_RPC_CMD_LEN 32
#define MAX_OPKG_UPDATE_URL_LEN 256
#define MAX_DEV_UPGRADE_URL_LEN 256
#define MAX_MD5_LEN 128
#define MAX_PLUGIN_VERSION_LEN  32

#define MAX_IPK_NAME_LEN 32
#define MAX_NODE_VALUE_LEN 32

typedef enum
{
    eRegister = 1,
    eIpkOperation,
    eQueryOperation,
    eOpkgOperation,
    ePluginOperation,
    eServiceOperation,
    eAlljoynGetOperation,
    eAlljoynSetOperation,
    eAlljoynUpdateDeviceId,
    eDevUpgrade,
    eInvalidParam
}eOpType;


typedef enum
{
    eInstall = 1,
    eUpdate,
    eDelete,
    eInvalidActionParam
}eActionType;

typedef struct plugin_info
{
    char url[MAX_OPKG_UPDATE_URL_LEN];
    char name[MAX_IPK_NAME_LEN];
    char version[MAX_PLUGIN_VERSION_LEN];
    int pluginId;
    struct plugin_info *next;
}plugin_info;

typedef struct ipk_info
{
    char op_ipk_name[MAX_IPK_NAME_LEN];
    char node_name_convert[MAX_IPK_NAME_LEN];
    char node_value[MAX_NODE_VALUE_LEN];
    int status;
    struct ipk_info *next;
}ipk_info;

typedef struct http_value
{
    int     rpc_flag;
    int     register_status;
    int     serial_num;
    int     opkg_update_flag;
    int     plugin_action_flag;
    int     real_ipk_num;
    char    rpc_cmd[MAX_RPC_CMD_LEN];
    char    opkg_update_url[MAX_OPKG_UPDATE_URL_LEN];
    struct plugin_info *plugin_head;
    struct ipk_info *ipk_name_head;
    char user_id[MAX_USERID_LEN];
    char device_id[MAX_DEVICEID_LEN];
    char device_sn[MAX_DEVICESN_LEN];
    char device_type[MAX_DEVICETYPE_LEN];  //temp add for alljoyn
    char *devData;
    char    dev_upgrade_url[MAX_DEV_UPGRADE_URL_LEN];
//    char version_id[MAX_VERSION_LEN];
    char    MD5[MAX_MD5_LEN];
    /*char    returncode[8];//200 OK or 500 */
}http_value;

/* task queue list */
typedef struct list_node
{
    struct list_node *pre;
    struct list_node *next;
}list_node;

/* task queue node */
typedef struct task_data_node
{
    struct list_node task_list_node;
    struct http_value data;
}task_data_node;

typedef struct fxIoT_head
{
    short int length;
    char cequence;
    char reserved;
}fxIoT_head;

#ifdef __cplusplus
}
#endif



#endif
