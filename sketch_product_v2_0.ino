#include <TroykaIMU.h>
#include <Adafruit_NeoPixel.h>
#include <cmath>
#include <stdlib.h>
#include <algorithm>

#define LED_PIN 20
#define LED_COUNT 2

float g = 3.1; // 3.1 or 3.5
int epsilon = 10;
int rnd_num;
float* ang;
int* tg;
int* ac;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Accelerometer accelerometer;


// Преобразование трех проекций (_data) ускорения для получения ~линейной зависимости яркости цвета от угла (data)
void linear_data(float* _data, int* data){
    data[0] = round((1 - sqrt((1 - abs(_data[0]) / g))) * 255);
    data[1] = round((1 - sqrt((1 - abs(_data[1]) / g))) * 255);
    data[2] = round((1 - sqrt((1 - abs(_data[2]) / g))) * 255);
}


// Получение данных с акселерометра для подачи на светодиод
int* gain_linear_ac(){
    float _ac[3];
    _ac[0] = accelerometer.readAX();
    _ac[1] = accelerometer.readAY();
    _ac[2] = accelerometer.readAZ();

    Serial.print(_ac[0]);
    Serial.print("\t");
    Serial.print(_ac[1]);
    Serial.print("\t");
    Serial.print(_ac[2]);
    Serial.print("\t\t");
    Serial.print(ang[0]);
    Serial.print("\t");
    Serial.print(ang[1]);
    Serial.print("\t");
    Serial.print(ang[2]);
    Serial.print("\t\t");

    vec_rotate(_ac, ang);

    for(int i = 0; i < 3; i++)
      if(_ac[i] > g)
        _ac[i] = g;
      else if(_ac[i] < -g)
        _ac[i] = -g;
    
    static int ac[3];
    linear_data(_ac, ac);

    return ac;
}


// Генерация достижимого целевого цвета
int* gen_random_target(int rnd_num){
    float _tg[3];
    _tg[0] = (rnd_num % (int)(g*10) + 1) / 10.0;

    if(_tg[0] == g){
      _tg[1] = 0;
      _tg[2] = 0;
    } 
    else{
      _tg[1] = (rand() % (int)round( sqrt(g*g - _tg[0]*_tg[0]) * 10 )) / 10.0;
      _tg[2] = sqrt(g*g - _tg[0]*_tg[0] - _tg[1]*_tg[1]);
    }

    random_replace(_tg, rnd_num);

    static int tg[3];
    linear_data(_tg, tg);
    return tg;
}


void random_replace(float* _tg, int rnd_num){
    if(rnd_num % 2 == 1)
      std::swap(_tg[0], _tg[1]);
    rnd_num = rand();
    if(rnd_num % 3 == 1){
      std::swap(_tg[0], _tg[2]);
      std::swap(_tg[0], _tg[1]);
    } else if(rnd_num % 3 == 2){
      std::swap(_tg[0], _tg[2]);
      std::swap(_tg[1], _tg[2]);
    }
}


float* gen_random_angles(){
    static float ang[3];
    ang[0] = (rand() % 314) / 100.0;
    ang[1] = (rand() % 314) / 100.0;
    ang[2] = (rand() % 314) / 100.0;
    return ang;
}


// Возвращает True, если мы попали в эпсилон-окрестность по двум цветам и в двойную эпсилон-окрестность по третьему
bool check(int* ac, int* tg, int epsilon){
    int count = 0;
    for(int i = 0; i < 3; i++)
      if(abs(ac[i] - tg[i]) <= epsilon)
        count += 2;
      else if(abs(ac[i] - tg[i]) <= 2*epsilon)
        count += 1;

    if(count >= 5)
      return 1;
    return 0;
}


// Функция поворота базиса в радианах by Максим
static void vec_rotate(float* vector, float* angles)
{
	float vector_new[3]{ 0, 0, 0 };
	// x координата:
	vector_new[0] = (vector[0] * cos(angles[1]) * cos(angles[2])) + (vector[1] * sin(angles[2]) * cos(angles[1])) + (vector[2] * -sin(angles[1]));
	// y координата:
	vector_new[1] = (vector[0] * (sin(angles[0]) * sin(angles[1]) * cos(angles[2]) - sin(angles[2]) * cos(angles[0]))) + 
		(vector[1] * (sin(angles[0]) * sin(angles[1]) * sin(angles[2]) + cos(angles[2]) * cos(angles[0]))) + (vector[2] * (sin(angles[0]) * cos(angles[1])));
	// z координата:
	vector_new[2] = (vector[0] * (sin(angles[0]) * sin(angles[2]) + sin(angles[1]) * cos(angles[0]) * cos(angles[2]))) +
		(vector[1] * (-sin(angles[0]) * cos(angles[2]) + sin(angles[1]) * sin(angles[2]) * cos(angles[0]))) + (vector[2] * (cos(angles[0]) * cos(angles[1])));
	// Запись в исходный указатель
	vector[0] = round(100 * vector_new[0])/100;
	vector[1] = round(100 * vector_new[1])/100;
	vector[2] = round(100 * vector_new[2])/100;
	return;
}


void setup() {
    strip.begin();
    strip.setBrightness(40);
    Serial.begin(9600);
    accelerometer.begin();
    accelerometer.setRange(AccelerometerRange::RANGE_4G);

    tg = gen_random_target(rand());
    ang = gen_random_angles();
}

void loop() {

    ac = gain_linear_ac();

    rnd_num = rand();

    if(check(ac, tg, epsilon)){
      tg = gen_random_target(rnd_num);
      ang = gen_random_angles();
    }

    Serial.print(tg[0]);
    Serial.print("\t");
    Serial.print(tg[1]);
    Serial.print("\t");
    Serial.print(tg[2]);
    Serial.print("\t\t");
    Serial.print(ac[0]);
    Serial.print("\t");
    Serial.print(ac[1]);
    Serial.print("\t");
    Serial.print(ac[2]);

    Serial.println();
    
    strip.setPixelColor(0, strip.Color(ac[0], ac[1], ac[2]));
    strip.setPixelColor(1, strip.Color(tg[0], tg[1], tg[2]));
    strip.show();

    delay(80);
}