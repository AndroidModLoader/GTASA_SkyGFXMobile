#include <stdint.h>

typedef struct CColourSet CColourSet;
struct CColourSet
{
	float ambr;
	float ambg;
	float ambb;
	float ambobjr;
	float ambobjg;
	float ambobjb;
	float ambBeforeBrightnessr;
	float ambBeforeBrightnessg;
	float ambBeforeBrightnessb;
	int16_t skytopr;
	int16_t skytopg;
	int16_t skytopb;
	int16_t skybotr;
	int16_t skybotg;
	int16_t skybotb;
	int16_t suncorer;
	int16_t suncoreg;
	int16_t suncoreb;
	int16_t suncoronar;
	int16_t suncoronag;
	int16_t suncoronab;
	float sunsz;
	float sprsz;
	float sprbght;
	int16_t shd;
	int16_t lightshd;
	int16_t poleshd;
	float farclp;
	float fogst;
	float lightonground;
	int16_t lowcloudr;
	int16_t lowcloudg;
	int16_t lowcloudb;
	int16_t fluffycloudr;
	int16_t fluffycloudg;
	int16_t fluffycloudb;
	float waterr;
	float waterg;
	float waterb;
	float watera;
	float postfx1r;
	float postfx1g;
	float postfx1b;
	float postfx1a;
	float postfx2r;
	float postfx2g;
	float postfx2b;
	float postfx2a;
	float cloudalpha;
	int intensityLimit;
	int16_t waterfogalpha;
	float directionalmult;
	float lodDistMult;

	// colorcycle grades here..
};