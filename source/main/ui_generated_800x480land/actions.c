#include "actions.h"
#include "screens.h"

void action_show_main_page(lv_event_t * e)
{
    (void)e;
    lv_scr_load(objects.screen_main);
}

void action_show_sub_main_page(lv_event_t * e)
{
    (void)e;
    lv_scr_load(objects.screen1);
}
