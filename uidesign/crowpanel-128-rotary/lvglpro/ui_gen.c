/**
 * @file ui_gen.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_gen.h"

#if defined(LV_USE_XML) && LV_USE_XML
#endif /* LV_USE_XML */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void check_font(lv_font_t ** font, const char * name);

/**********************
 *  STATIC VARIABLES
 **********************/

static uint32_t ui_target = UI_TARGET_ALL;

/*----------------
 * Translations
 *----------------*/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/*--------------------
 *  Permanent screens
 *-------------------*/

/*----------------
 * Fonts
 *----------------*/

lv_font_t * montserrat_12;
extern lv_font_t montserrat_12_data;
lv_font_t * montserrat_14;
extern lv_font_t montserrat_14_data;
lv_font_t * montserrat_16;
extern lv_font_t montserrat_16_data;
lv_font_t * montserrat_18;
extern lv_font_t montserrat_18_data;
lv_font_t * montserrat_20;
extern lv_font_t montserrat_20_data;
lv_font_t * montserrat_22;
extern lv_font_t montserrat_22_data;
lv_font_t * montserrat_40;
extern lv_font_t montserrat_40_data;
lv_font_t * fontawesome_20;
extern lv_font_t fontawesome_20_data;
lv_font_t * fontawesome_22;
extern lv_font_t fontawesome_22_data;

/*----------------
 * Images
 *----------------*/

/* Targets: any */
const void * img_cblabs_logo = NULL;
extern const void * img_cblabs_logo_data;

/*----------------
 * Global styles
 *----------------*/

lv_style_t transparent_container;

/*----------------
 * Subjects
 *----------------*/

lv_subject_t sys_boot_progress;
lv_subject_t sys_boot_status;
lv_subject_t sys_app_title;
lv_subject_t sys_error_visible;
lv_subject_t sys_setup_visible;
lv_subject_t sys_ota_visible;
lv_subject_t sys_busy_visible;
lv_subject_t sys_msg_visible;
lv_subject_t sys_error_title;
lv_subject_t sys_error_message;
lv_subject_t sys_setup_ssid;
lv_subject_t sys_setup_url;
lv_subject_t sys_ota_status;
lv_subject_t sys_ota_progress;
lv_subject_t sys_ota_percent;
lv_subject_t sys_busy_message;
lv_subject_t sys_msg_title;
lv_subject_t sys_msg_body;
lv_subject_t sys_diag_arc_value;
lv_subject_t sys_diag_category;
lv_subject_t sys_diag_value;
lv_subject_t sys_diag_counter;
lv_subject_t sys_diag_status;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ui_init_gen(const char * asset_path)
{

    /*----------------
     * Fonts
     *----------------*/

    /* Targets: any */

    #if UI_CHECK_COMPILE_TARGET(UI_TARGET_ALL)
    if (ui_check_target(UI_TARGET_ALL)) {
        if (!montserrat_12) {
            /* montserrat_12 */
            /* get font 'montserrat_12' from a C array */
            montserrat_12 = &montserrat_12_data;

        }
        if (!montserrat_14) {
            /* montserrat_14 */
            /* get font 'montserrat_14' from a C array */
            montserrat_14 = &montserrat_14_data;

        }
        if (!montserrat_16) {
            /* montserrat_16 */
            /* get font 'montserrat_16' from a C array */
            montserrat_16 = &montserrat_16_data;

        }
        if (!montserrat_18) {
            /* montserrat_18 */
            /* get font 'montserrat_18' from a C array */
            montserrat_18 = &montserrat_18_data;

        }
        if (!montserrat_20) {
            /* montserrat_20 */
            /* get font 'montserrat_20' from a C array */
            montserrat_20 = &montserrat_20_data;

        }
        if (!montserrat_22) {
            /* montserrat_22 */
            /* get font 'montserrat_22' from a C array */
            montserrat_22 = &montserrat_22_data;

        }
        if (!montserrat_40) {
            /* montserrat_40 */
            /* get font 'montserrat_40' from a C array */
            montserrat_40 = &montserrat_40_data;

        }
        if (!fontawesome_20) {
            /* fontawesome_20 */
            /* get font 'fontawesome_20' from a C array */
            fontawesome_20 = &fontawesome_20_data;

        }
        if (!fontawesome_22) {
            /* fontawesome_22 */
            /* get font 'fontawesome_22' from a C array */
            fontawesome_22 = &fontawesome_22_data;

        }
    }
    #endif

    /*----------------
     * Images
     *----------------*/

    /* Targets: any */
    #if UI_CHECK_COMPILE_TARGET(UI_TARGET_ALL)
    if (ui_check_target(UI_TARGET_ALL)) {
        /* img_cblabs_logo */
        if (!img_cblabs_logo) {
            img_cblabs_logo = &img_cblabs_logo_data;
        }
    }
    #endif

    /*----------------
     * Global styles
     *----------------*/

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&transparent_container);

        lv_style_set_bg_opa(&transparent_container, 0);
        lv_style_set_border_width(&transparent_container, 0);
        lv_style_set_pad_all(&transparent_container, 0);

        style_inited = true;
    }

    /*----------------
     * Subjects
     *----------------*/
    lv_subject_init_int(&sys_boot_progress, 0);
    static char boot_status_buf[UI_SUBJECT_STRING_LENGTH];
    static char boot_status_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_boot_status,
                           boot_status_buf,
                           boot_status_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "Initialising..."
                          );
    static char app_title_buf[UI_SUBJECT_STRING_LENGTH];
    static char app_title_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_app_title,
                           app_title_buf,
                           app_title_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "ESP32 App"
                          );
    lv_subject_init_int(&sys_error_visible, 0);
    lv_subject_init_int(&sys_setup_visible, 0);
    lv_subject_init_int(&sys_ota_visible, 0);
    lv_subject_init_int(&sys_busy_visible, 0);
    lv_subject_init_int(&sys_msg_visible, 0);
    static char error_title_buf[UI_SUBJECT_STRING_LENGTH];
    static char error_title_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_error_title,
                           error_title_buf,
                           error_title_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "System Error"
                          );
    static char error_message_buf[UI_SUBJECT_STRING_LENGTH];
    static char error_message_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_error_message,
                           error_message_buf,
                           error_message_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           ""
                          );
    static char setup_ssid_buf[UI_SUBJECT_STRING_LENGTH];
    static char setup_ssid_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_setup_ssid,
                           setup_ssid_buf,
                           setup_ssid_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "CBLabs-Setup"
                          );
    static char setup_url_buf[UI_SUBJECT_STRING_LENGTH];
    static char setup_url_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_setup_url,
                           setup_url_buf,
                           setup_url_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "192.168.4.1"
                          );
    static char ota_status_buf[UI_SUBJECT_STRING_LENGTH];
    static char ota_status_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_ota_status,
                           ota_status_buf,
                           ota_status_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "Downloading..."
                          );
    lv_subject_init_int(&sys_ota_progress, 0);
    static char ota_percent_buf[UI_SUBJECT_STRING_LENGTH];
    static char ota_percent_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_ota_percent,
                           ota_percent_buf,
                           ota_percent_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "0%"
                          );
    static char busy_message_buf[UI_SUBJECT_STRING_LENGTH];
    static char busy_message_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_busy_message,
                           busy_message_buf,
                           busy_message_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "Please wait..."
                          );
    static char msg_title_buf[UI_SUBJECT_STRING_LENGTH];
    static char msg_title_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_msg_title,
                           msg_title_buf,
                           msg_title_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           ""
                          );
    static char msg_body_buf[UI_SUBJECT_STRING_LENGTH];
    static char msg_body_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_msg_body,
                           msg_body_buf,
                           msg_body_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           ""
                          );
    lv_subject_init_int(&sys_diag_arc_value, 0);
    static char diag_category_buf[UI_SUBJECT_STRING_LENGTH];
    static char diag_category_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_diag_category,
                           diag_category_buf,
                           diag_category_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "TEMPERATURE"
                          );
    static char diag_value_buf[UI_SUBJECT_STRING_LENGTH];
    static char diag_value_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_diag_value,
                           diag_value_buf,
                           diag_value_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "--"
                          );
    static char diag_counter_buf[UI_SUBJECT_STRING_LENGTH];
    static char diag_counter_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_diag_counter,
                           diag_counter_buf,
                           diag_counter_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           "0 / 0"
                          );
    static char diag_status_buf[UI_SUBJECT_STRING_LENGTH];
    static char diag_status_prev_buf[UI_SUBJECT_STRING_LENGTH];
    lv_subject_init_string(&sys_diag_status,
                           diag_status_buf,
                           diag_status_prev_buf,
                           UI_SUBJECT_STRING_LENGTH,
                           ""
                          );

    /*----------------
     * Translations
     *----------------*/

#if defined(LV_USE_XML) && LV_USE_XML
    /* Register widgets */

    /* Check all fonts / default if needed. This prevents fonts that are used in one target but
       defined in another from causing assertion failures during rendering of the Preview. */
    check_font(&montserrat_12, "montserrat_12");
    check_font(&montserrat_14, "montserrat_14");
    check_font(&montserrat_16, "montserrat_16");
    check_font(&montserrat_18, "montserrat_18");
    check_font(&montserrat_20, "montserrat_20");
    check_font(&montserrat_22, "montserrat_22");
    check_font(&montserrat_40, "montserrat_40");
    check_font(&fontawesome_20, "fontawesome_20");
    check_font(&fontawesome_22, "fontawesome_22");

    /* Register fonts */
    lv_xml_register_font(NULL, "montserrat_12", montserrat_12);
    lv_xml_register_font(NULL, "montserrat_14", montserrat_14);
    lv_xml_register_font(NULL, "montserrat_16", montserrat_16);
    lv_xml_register_font(NULL, "montserrat_18", montserrat_18);
    lv_xml_register_font(NULL, "montserrat_20", montserrat_20);
    lv_xml_register_font(NULL, "montserrat_22", montserrat_22);
    lv_xml_register_font(NULL, "montserrat_40", montserrat_40);
    lv_xml_register_font(NULL, "fontawesome_20", fontawesome_20);
    lv_xml_register_font(NULL, "fontawesome_22", fontawesome_22);

    /* Register subjects */
    lv_xml_register_subject(NULL, "sys_boot_progress", &sys_boot_progress);
    lv_xml_register_subject(NULL, "sys_boot_status", &sys_boot_status);
    lv_xml_register_subject(NULL, "sys_app_title", &sys_app_title);
    lv_xml_register_subject(NULL, "sys_error_visible", &sys_error_visible);
    lv_xml_register_subject(NULL, "sys_setup_visible", &sys_setup_visible);
    lv_xml_register_subject(NULL, "sys_ota_visible", &sys_ota_visible);
    lv_xml_register_subject(NULL, "sys_busy_visible", &sys_busy_visible);
    lv_xml_register_subject(NULL, "sys_msg_visible", &sys_msg_visible);
    lv_xml_register_subject(NULL, "sys_error_title", &sys_error_title);
    lv_xml_register_subject(NULL, "sys_error_message", &sys_error_message);
    lv_xml_register_subject(NULL, "sys_setup_ssid", &sys_setup_ssid);
    lv_xml_register_subject(NULL, "sys_setup_url", &sys_setup_url);
    lv_xml_register_subject(NULL, "sys_ota_status", &sys_ota_status);
    lv_xml_register_subject(NULL, "sys_ota_progress", &sys_ota_progress);
    lv_xml_register_subject(NULL, "sys_ota_percent", &sys_ota_percent);
    lv_xml_register_subject(NULL, "sys_busy_message", &sys_busy_message);
    lv_xml_register_subject(NULL, "sys_msg_title", &sys_msg_title);
    lv_xml_register_subject(NULL, "sys_msg_body", &sys_msg_body);
    lv_xml_register_subject(NULL, "sys_diag_arc_value", &sys_diag_arc_value);
    lv_xml_register_subject(NULL, "sys_diag_category", &sys_diag_category);
    lv_xml_register_subject(NULL, "sys_diag_value", &sys_diag_value);
    lv_xml_register_subject(NULL, "sys_diag_counter", &sys_diag_counter);
    lv_xml_register_subject(NULL, "sys_diag_status", &sys_diag_status);

    /* Register callbacks */
    lv_xml_register_event_cb(NULL, "ui_boot_fade_overlay_loaded", ui_boot_fade_overlay_loaded);
    lv_xml_register_event_cb(NULL, "ui_system_dismiss_clicked", ui_system_dismiss_clicked);
#endif

    /* Register all the global assets so that they won't be created again when globals.xml is parsed.
     * While running in the editor skip this step to update the preview when the XML changes */
#if defined(LV_USE_XML) && LV_USE_XML && !defined(LV_EDITOR_PREVIEW)
    /* Register images */
    lv_xml_register_image(NULL, "img_cblabs_logo", img_cblabs_logo);
#endif

#if !defined(LV_USE_XML) || LV_USE_XML == 0
    /*--------------------
     *  Permanent screens
     *-------------------*/
    /* If XML is enabled it's assumed that the permanent screens are created
     * manually from XML using lv_xml_create() */
#endif
}

void ui_set_target(uint32_t target)
{
    ui_target = target;
}

uint32_t ui_get_target(void)
{
    return ui_target;
}

bool ui_check_target(uint32_t target)
{
    return (ui_target & target) ? true : false;
}

/* Callbacks */
#if defined(LV_EDITOR_PREVIEW)
void __attribute__((weak)) ui_boot_fade_overlay_loaded(lv_event_t * e)
{
    LV_UNUSED(e);
    LV_LOG("ui_boot_fade_overlay_loaded was called\n");
}
void __attribute__((weak)) ui_system_dismiss_clicked(lv_event_t * e)
{
    LV_UNUSED(e);
    LV_LOG("ui_system_dismiss_clicked was called\n");
}
#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void check_font(lv_font_t ** font, const char * name)
{
    if (!(*font)) {
        *font = (lv_font_t *)LV_FONT_DEFAULT;
        LV_LOG_WARN("font `%s` was not set. Using `LV_FONT_DEFAULT` instead", name);
    }
}