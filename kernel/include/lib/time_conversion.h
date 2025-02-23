#pragma once

char* fsm_timePrintable(FSM_TIME time);
void fsm_convertUnix(uint32_t unix_time, FSM_TIME* time);
size_t fsm_DateConvertToUnix(FSM_TIME time);