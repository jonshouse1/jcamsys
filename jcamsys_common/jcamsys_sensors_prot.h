// Prototypes for jcamsys_sensors.c

//Prototypes
void jc_sensor_newvalue(struct jcamsys_sensorstate* senss, struct jcamsys_sens_value* msgv);
int jc_sensor_valid(int senstype, int sensor);
void jc_sensor_mark_time(struct jcamsys_sensorstate* senss, int senstype, int sensor);
void jc_sensor_mark_active(struct jcamsys_sensorstate* senss, int senstype, int sensor);
void jc_sensor_mark_inactive(struct jcamsys_sensorstate* senss, int senstype, int sensor);



