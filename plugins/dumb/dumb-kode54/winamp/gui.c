
#include <stdio.h>
#include <windows.h>
//#include <mmreg.h>
//#include <msacm.h>
#include <math.h>
#include <commctrl.h>

#include "in_duh.h"
#include "resource.h"

#include "gui.h"


/* Registry settings */
#define REGISTRY_KEY "Software\\Winamp\\DUH Plug-in"
static HKEY registry = INVALID_HANDLE_VALUE;

/* Some default values for config */
int config_bits_per_sample = 16;
int config_frequency = 44100;
int config_stereo = CHANNEL_STEREO;
int config_resampling = RESAMPLING_CUBIC;
int config_buffer_size = 8192;
int config_thread_priority = PRIORITY_HIGH;


/*************************
 * Registry manipulation */

static int read_registry(char const *name, unsigned long type, void *ptr, unsigned long size)
{
	unsigned long reg_type;

	if (registry == INVALID_HANDLE_VALUE ||
		RegQueryValueEx(registry, name, 0, &reg_type, ptr, &size) != ERROR_SUCCESS
		|| reg_type != type)
		
		return -1;

	return 0;
}


static int write_registry(char const *name, unsigned long type, void *ptr, unsigned long size)
{
	if (registry == INVALID_HANDLE_VALUE
		|| RegSetValueEx(registry, name, 0, type, ptr, size) != ERROR_SUCCESS)
		
		return -1;

	return 0;
}


# define LOAD_REG_INT(name, var, defaultv) (read_registry(name, REG_DWORD,  \
	&(var), sizeof(var)) == -1 ? ((var) = (defaultv)) : (var))

# define LOAD_REG_STRING(name, var, defaultv) (read_registry(name, REG_SZ,  \
	(var), sizeof(var)) == -1 ? strcpy(var, (defaultv)) : (var))

# define SAVE_REG_INT(name, var) (write_registry(name, REG_DWORD, &(var), sizeof(var)))
# define SAVE_REG_STRING(name, var) (write_registry(name, REG_SZ, (var), sizeof(var)))


/*********************
 * Range check       */

#define CHECK_RANGE(x, a, b) x = ((x < (a)) ? (a) : ((x > (b)) ? (b) : x))


/*********************
 * Config Dialog Box */


static INT_PTR CALLBACK config_dialog(HWND dialog, UINT message,
								   WPARAM wparam, LPARAM lparam)
{
	int which;
	int temp;
	static int old_slider1 = 0;
	char str[64];

	(void)lparam;

	switch (message) {
	case WM_INITDIALOG:

		/* Ok, now we need to set up the dialog's controls
		 * to match the current config 
		 */

		/* Channels */
		CheckDlgButton(dialog, IDC_STEREO, config_stereo == CHANNEL_STEREO ? BST_CHECKED : BST_UNCHECKED);

		/* Bits per sample */
		switch (config_bits_per_sample) {
			case  8: which = IDC_8BPS;  break;
			case 16: which = IDC_16BPS; break;
			default:
			which = 0;
		}
		if (which)
			CheckRadioButton(dialog, IDC_8BPS, IDC_16BPS, which);

		/* Resampling method */
		switch (config_resampling) {
			case  RESAMPLING_ALIASING:  which = IDC_ALIASING;  break;
			case  RESAMPLING_LINEAR:    which = IDC_LINEAR;    break;
			case  RESAMPLING_LINEAR2:   which = IDC_LINEAR_LOW_PASS;   break;
			case  RESAMPLING_QUADRATIC: which = IDC_QUADRATIC; break;
			case  RESAMPLING_CUBIC:     which = IDC_CUBIC;     break;
			default:
			which = 0;
		}
		if (which)
			CheckRadioButton(dialog, IDC_ALIASING, IDC_CUBIC, which);

		/* Frequency */
		switch (config_frequency) {
			case  11025:  which = IDC_11KHZ; break;
			case  22050:  which = IDC_22KHZ; break;
			case  44100:  which = IDC_44KHZ; break;
			case  48000:  which = IDC_48KHZ; break;
			default:
			which = 0;
		}
		if (which)
			CheckRadioButton(dialog, IDC_11KHZ, IDC_48KHZ, which);

		/* Buffer size - 1 KB -> 32 KB slider */
		old_slider1 = (int)(log(config_buffer_size) / log(2)) - 10;
		SendDlgItemMessage(dialog, IDC_BUFFERSIZE, TBM_SETRANGE, FALSE, MAKELONG(0, 5));
		SendDlgItemMessage(dialog, IDC_BUFFERSIZE, TBM_SETPOS, TRUE, old_slider1);
		sprintf(str, "%i KS", config_buffer_size / 1024);
		SetDlgItemText(dialog, IDC_BUFFERSIZE2, str);

		/* Thread Priority */
		SendDlgItemMessage(dialog, IDC_THREAD_PRI, TBM_SETRANGE, FALSE, MAKELONG(0, 2));
		SendDlgItemMessage(dialog, IDC_THREAD_PRI, TBM_SETPOS,   TRUE,  config_thread_priority);

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wparam)) {

		case IDC_DEFAULT:
			/* Load default settings */

			/* Channels */
			CheckDlgButton(dialog, IDC_STEREO, BST_CHECKED);

			/* Bits per sample */
			CheckRadioButton(dialog, IDC_8BPS, IDC_16BPS, IDC_16BPS);

			/* Resampling method */
			CheckRadioButton(dialog, IDC_ALIASING, IDC_CUBIC, IDC_LINEAR_LOW_PASS);

			/* Frequency */
			CheckRadioButton(dialog, IDC_11KHZ, IDC_48KHZ, IDC_44KHZ);

			/* Buffer size - 1 KB -> 32 KB slider */
			SendDlgItemMessage(dialog, IDC_BUFFERSIZE, TBM_SETPOS, TRUE, 3);
			sprintf(str, "%i KS", 8192 / 1024);
			SetDlgItemText(dialog, IDC_BUFFERSIZE2, str);

			/* Thread Priority */
			SendDlgItemMessage(dialog, IDC_THREAD_PRI, TBM_SETPOS,   TRUE,  PRIORITY_HIGH);

			return TRUE;

		case IDC_NICEST:
			/* Load nicest settings */

			/* Channels */
			CheckDlgButton(dialog, IDC_STEREO, BST_CHECKED);

			/* Bits per sample */
			CheckRadioButton(dialog, IDC_8BPS, IDC_16BPS, IDC_16BPS);

			/* Resampling method */
			CheckRadioButton(dialog, IDC_ALIASING, IDC_CUBIC, IDC_CUBIC);

			/* Frequency */
			CheckRadioButton(dialog, IDC_11KHZ, IDC_48KHZ, IDC_48KHZ);

			/* Buffer size - 1 KB -> 32 KB slider */
			SendDlgItemMessage(dialog, IDC_BUFFERSIZE, TBM_SETPOS, TRUE, 3);
			sprintf(str, "%i KS", 8192 / 1024);
			SetDlgItemText(dialog, IDC_BUFFERSIZE2, str);

			/* Thread Priority */
			SendDlgItemMessage(dialog, IDC_THREAD_PRI, TBM_SETPOS,   TRUE,  PRIORITY_HIGH);

			return TRUE;

		case IDC_FASTEST:
			/* Load fastest settings */

			/* Channels */
			CheckDlgButton(dialog, IDC_STEREO, BST_UNCHECKED);

			/* Bits per sample */
			CheckRadioButton(dialog, IDC_8BPS, IDC_16BPS, IDC_8BPS);

			/* Resampling method */
			CheckRadioButton(dialog, IDC_ALIASING, IDC_CUBIC, IDC_ALIASING);

			/* Frequency */
			CheckRadioButton(dialog, IDC_11KHZ, IDC_48KHZ, IDC_11KHZ);

			/* Buffer size - 1 KB -> 32 KB slider */
			SendDlgItemMessage(dialog, IDC_BUFFERSIZE, TBM_SETPOS, TRUE, 3);
			sprintf(str, "%i KS", 8192 / 1024);
			SetDlgItemText(dialog, IDC_BUFFERSIZE2, str);

			/* Thread Priority */
			SendDlgItemMessage(dialog, IDC_THREAD_PRI, TBM_SETPOS,   TRUE,  PRIORITY_HIGH);

			return TRUE;

		case IDC_OK:
			/* Read back configuration */
			config_stereo = (IsDlgButtonChecked(dialog, IDC_STEREO) == BST_CHECKED) ? CHANNEL_STEREO : CHANNEL_MONO;
			config_bits_per_sample = (IsDlgButtonChecked(dialog, IDC_8BPS) == BST_CHECKED) ? 8 : 16;
			config_resampling = (IsDlgButtonChecked(dialog, IDC_ALIASING) == BST_CHECKED) ? RESAMPLING_ALIASING
				: (IsDlgButtonChecked(dialog, IDC_LINEAR) == BST_CHECKED) ? RESAMPLING_LINEAR
				: (IsDlgButtonChecked(dialog, IDC_LINEAR_LOW_PASS) == BST_CHECKED) ? RESAMPLING_LINEAR2
				: (IsDlgButtonChecked(dialog, IDC_QUADRATIC) == BST_CHECKED) ? RESAMPLING_QUADRATIC
				: RESAMPLING_CUBIC;
			config_frequency = (IsDlgButtonChecked(dialog, IDC_11KHZ) == BST_CHECKED) ? 11025
				: (IsDlgButtonChecked(dialog, IDC_22KHZ) == BST_CHECKED) ? 22050
				: (IsDlgButtonChecked(dialog, IDC_44KHZ) == BST_CHECKED) ? 44100
				: 48000;
			config_buffer_size = 1 << (SendDlgItemMessage(dialog, IDC_BUFFERSIZE, TBM_GETPOS, 0, 0) + 10);

			config_thread_priority = SendDlgItemMessage(dialog, IDC_THREAD_PRI, TBM_GETPOS, 0, 0);

		case IDCANCEL:
		case IDC_CANCEL:
		case IDCLOSE:
			EndDialog(dialog, wparam);
			return TRUE;
		}
		break;
	default:
		temp = 1 << (SendDlgItemMessage(dialog, IDC_BUFFERSIZE, TBM_GETPOS, 0, 0) + 10);
		if (temp != old_slider1) {
			old_slider1 = temp;
			sprintf(str, "%i KS", temp / 1024);
			SetDlgItemText(dialog, IDC_BUFFERSIZE2, str);
			return TRUE;
		}
		break;
	}
	return FALSE;
}



/* DUH Config dialog */
void config(HWND hwndParent) {

	if (DialogBox(mod.hDllInstance, MAKEINTRESOURCE(IDD_CONFIG),
		hwndParent, config_dialog) == IDC_OK) {

		SAVE_REG_INT("Stereo",         config_stereo);
		SAVE_REG_INT("Frequency",      config_frequency);
		SAVE_REG_INT("Resampling",     config_resampling);
		SAVE_REG_INT("BitsPerSample",  config_bits_per_sample);
		SAVE_REG_INT("BufferSize",     config_buffer_size);
		SAVE_REG_INT("ThreadPriority", config_thread_priority);
	}
}


void config_init(void) {

	/* Load config from registry */

	if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, "",
		REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, 0,
		&registry, 0) != ERROR_SUCCESS)
		
		registry = INVALID_HANDLE_VALUE;

	LOAD_REG_INT("Stereo",         config_stereo,     CHANNEL_STEREO);
	LOAD_REG_INT("Frequency",      config_frequency,  44100);
	LOAD_REG_INT("Resampling",     config_resampling, RESAMPLING_CUBIC);
	LOAD_REG_INT("BitsPerSample",  config_bits_per_sample, 16);
	LOAD_REG_INT("BufferSize",     config_buffer_size, 8192);
	LOAD_REG_INT("ThreadPriority", config_thread_priority, PRIORITY_HIGH);

	CHECK_RANGE(config_stereo, CHANNEL_MONO, CHANNEL_STEREO);
	if (   config_frequency != 11025
		&& config_frequency != 22050
		&& config_frequency != 44100
		&& config_frequency != 48000)
		config_frequency = 44100;
	CHECK_RANGE(config_resampling, RESAMPLING_ALIASING, RESAMPLING_CUBIC);
	if (   config_bits_per_sample != 8
		&& config_bits_per_sample != 16)
		config_bits_per_sample = 16;
	CHECK_RANGE(config_buffer_size, 1024, 32768);
	CHECK_RANGE(config_thread_priority, PRIORITY_NORMAL, PRIORITY_HIGHEST);


	return;
}



void config_quit(void) {

	/* Close registry key */
	if (registry != INVALID_HANDLE_VALUE) {
		RegCloseKey(registry);
		registry = INVALID_HANDLE_VALUE;
	}

	return;
}



/* About box, yay! */
void about(HWND hwndParent) {
	MessageBox(hwndParent, "DUH! Winamp Plugin\n Version " VERSION " (x86)\n", 
		"About:", MB_OK | MB_ICONINFORMATION);
}

