#include "actions.h"
#include "ui.h"

void action_show_main_page(lv_event_t * e)
{
    (void)e;
    loadScreen(SCREEN_ID_SCREEN_MAIN);
}

void action_show_sub_main_page(lv_event_t * e)
{
    (void)e;
    loadScreen(SCREEN_ID_SCREEN1);
}
