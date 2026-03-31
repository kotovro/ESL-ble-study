#ifndef BUTTON_UTILS
#define BUTTON_UTILS

typedef void  (*Double_Click_Executor)();
typedef void  (*Button_Press_Executor)();

void init_button(Double_Click_Executor double_click_executor, Button_Press_Executor press_executor);

#endif