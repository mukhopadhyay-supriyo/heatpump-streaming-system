# from pydantic import BaseModel
from typing import Any

SIGNALS: dict[int, tuple[str, str]] = {
    0: ("Record_Id", "Record_Id"),
    1: ("Device_Id", "Device_Id"),
    2: ("Data_Length", "Data_Length"),
    3: ("AC_Input_0", "AC_Input_0"),
    4: ("AC_Input_1", "AC_Input_1"),
    5: ("AC_Input_2", "AC_Input_2"),
    6: ("Suction_Temperature", "Suction_Temperature"),
    7: ("Discharge_Temperature", "Discharge_Temperature"),
    8: ("EXP_IN_Temperature", "EXP_IN_Temperature"),
    9: ("EXP_OUT_Temperature", "EXP_OUT_Temperature"),
    10: ("Ambient_Temperture", "Ambient_Temperture"),
    11: ("T6", "T6"),
    12: ("T7", "T7"),
    13: ("PCB_NTC", "PCB_NTC"),
    14: ("Flow_1", "Flow_1"),
    15: ("Flow_2", "Flow_2"),
    16: ("Output_Flag_1", "Output_Flag_1"),
    17: ("Output_Flag_2", "Output_Flag_2"),
    18: ("Output_Flag_3", "Output_Flag_3"),
    19: ("Fan_Tach", "Fan_Tach"),
    20: ("Stepper_Position", "Stepper_Position"),
    21: ("Flow_Rate", "Flow_Rate"),
    22: ("Comp_Speed_Target", "Comp_Speed_Target"),
    23: ("Comp_Speed_Actual", "Comp_Speed_Actual"),
    24: ("Discharge_Pressure", "Discharge_Pressure"),
    25: ("Suction_Pressure", "Suction_Pressure"),
    26: ("Fan_Speed_Target", "Fan_Speed_Target"),
    27: ("Superheat", "Superheat"),
    28: ("Subcool", "Subcool"),
    29: ("Pump_Speed_Target", "Pump_Speed_Target"),
    30: ("System_Flow_Rate", "System_Flow_Rate"),
    31: ("State_Changes_Per_6s", "State_Changes_Per_6s"),
    32: ("Flow_Rate_L_Per_Min_Times_10", "Flow_Rate_L_Per_Min_Times_10"),
    33: ("Sika_Flow_Rate", "Sika_Flow_Rate"),
    34: ("Evaporation_Temperature", "Evaporation_Temperature"),
    35: ("Condensing_Temperature", "Condensing_Temperature"),
    36: ("System_Flow_Temperature", "System_Flow_Temperature"),
    37: ("System_Return_Temperature", "System_Return_Temperature"),
    38: ("P_Flow_Setpoint", "P_Flow_Setpoint"),
    39: ("D_Ana_Val", "D_Ana_Val"),
    40: ("Superheat_Error", "Superheat_Error"),
    41: ("Discharge_Temperature_Error", "Discharge_Temperature_Error"),
    42: ("P_EEV_Start_Position", "P_EEV_Start_Position"),
    43: ("Fan_State", "Fan_State"),
    44: ("Comp_State", "Comp_State"),
    45: ("Comp_Timer", "Comp_Timer"),
    46: ("EEV_State", "EEV_State"),
    47: ("EEV_Timer", "EEV_Timer"),
    48: ("EEV_Threshold_Timer", "EEV_Threshold_Timer"),
    49: ("Pump_APP_State", "Pump_APP_State"),
    50: ("Pump_Timer", "Pump_Timer"),
    51: ("Demand_1", "Demand_1"),
    52: ("P_EEV_Open_Time", "P_EEV_Open_Time"),
    53: ("P_KP", "P_KP"),
    54: ("P_Superheat_Error_Scalor", "P_Superheat_Error_Scalor"),
    55: ("P_EEV_Threshold_Check_Time", "P_EEV_Threshold_Check_Time"),
    56: ("P_Fan_Auto", "P_Fan_Auto"),
    57: ("pump_mode", "pump_mode"),
    58: ("pump_rpm_set", "pump_rpm_set"),
    59: ("Time_stamp", "Time_stamp"),
    60: ("Event_Id", "SequenceNumber"),
}

# print(SIGNALS[0][0])

# for i, col in enumerate(SIGNALS):
#     print(SIGNALS[i][0])..

update_columns = [
    SIGNALS[i][0]
    for i, column in enumerate(SIGNALS)
    if SIGNALS[i][0] not in ("Device_Id", "Time_stamp")
]


print(update_columns)
