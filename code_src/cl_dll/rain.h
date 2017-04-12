#ifndef __RAIN_H__
#define __RAIN_H__

#define DRIPSPEED	900		// �������� ������� ������ (���� � ���)
#define SNOWSPEED	200		// �������� ������� ��������
#define SNOWFADEDIST	80

#define MAXDRIPS	2000	// ����� ������ (����� ��������� ��� �������������)
#define MAXFX		2000	// ����� �������������� ������ (����� �� ���� � �.�.)

#define DRIP_SPRITE_HALFHEIGHT	46
#define DRIP_SPRITE_HALFWIDTH	8
#define SNOW_SPRITE_HALFSIZE	3

// "������" ����� �� ����, �� �������� �� ������������ �� �������
#define MAXRINGHALFSIZE		25	
 


typedef struct
{
	int			dripsPerSecond;
	float		distFromPlayer;
	float		windX, windY;
	float		randX, randY;
	int			weatherMode;	// 0 - snow, 1 - rain
	float		globalHeight;
} rain_properties;


typedef struct cl_drip
{
	float		birthTime;
	float		minHeight;	// ����� ����� ���������� �� ���� ������.
	vec3_t		origin;
	float		alpha;

	float		xDelta;		// side speed
	float		yDelta;
	int			landInWater;
	
//	cl_drip*		p_Next;		// next drip in chain
//	cl_drip*		p_Prev;		// previous drip in chain
} cl_drip_t;

typedef struct cl_rainfx
{
	float		birthTime;
	float		life;
	vec3_t		origin;
	float		alpha;
	
//	cl_rainfx*		p_Next;		// next fx in chain
//	cl_rainfx*		p_Prev;		// previous fx in chain
} cl_rainfx_t;

 
void ProcessRain( void );
void ProcessFXObjects( void );
void ResetRain( void );
void InitRain( void );

#endif

