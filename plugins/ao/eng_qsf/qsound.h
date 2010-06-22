/*********************************************************

    Capcom Q-Sound system

*********************************************************/

#ifndef __QSOUND_H__
#define __QSOUND_H__

#define QSOUND_CLOCK    4000000   /* default 4MHz clock */

struct QSound_interface 
{
	int clock;			/* clock */
	char *sample_rom;		/* sample data */
};

int  qsound_sh_start( struct QSound_interface *qsintf );
void qsound_sh_stop( void );

void qsound_data_h_w(int data);
void qsound_data_l_w(int data);
void qsound_cmd_w(int data);
int qsound_status_r(void);
void qsound_update( int num, INT16 **buffer, int length );

#endif /* __QSOUND_H__ */
