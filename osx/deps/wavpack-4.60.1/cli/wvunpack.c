////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2009 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wvunpack.c

// This is the main module for the WavPack command-line decompressor.

#if defined(WIN32)
#include <windows.h>
#include <io.h>
#else
#if defined(__OS2__)
#define INCL_DOSPROCESS
#include <os2.h>
#include <io.h>
#endif
#include <sys/stat.h>
#include <sys/param.h>
#include <locale.h>
#include <iconv.h>
#if defined (__GNUC__)
#include <unistd.h>
#include <glob.h>
#endif
#endif

#if defined(__GNUC__) && !defined(WIN32)
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "wavpack.h"
#include "utils.h"
#include "md5.h"

#ifdef WIN32
#define strdup(x) _strdup(x)
#define fileno _fileno
#endif

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
static char *strdup (const char *s)
 { char *d = malloc (strlen (s) + 1); return strcpy (d, s); }
#endif

///////////////////////////// local variable storage //////////////////////////

static const char *sign_on = "\n"
" WVUNPACK  Hybrid Lossless Audio Decompressor  %s Version %s\n"
" Copyright (c) 1998 - 2009 Conifer Software.  All Rights Reserved.\n\n";

static const char *usage =
#if defined (WIN32)
" Usage:   WVUNPACK [-options] [@]infile[.wv]|- [[@]outfile[.wav]|outpath|-]\n"
#else
" Usage:   WVUNPACK [-options] [@]infile[.wv]|- [...] [-o [@]outfile[.wav]|outpath|-]\n"
#endif
"             (infile may contain wildcards: ?,*)\n\n"
" Options: -b  = blindly decode all stream blocks & ignore length info\n"
"          -c  = extract cuesheet only to stdout (no audio decode)\n"
"               (note: equivalent to -x \"cuesheet\")\n"
"          -cc = extract cuesheet file (.cue) in addition to audio file\n"
"               (note: equivalent to -xx \"cuesheet=%a.cue\")\n"
"          -d  = delete source file if successful (use with caution!)\n"
"          --help = this help display\n"
"          -i  = ignore .wvc file (forces hybrid lossy decompression)\n"
#if defined (WIN32) || defined (__OS2__)
"          -l  = run at low priority (for smoother multitasking)\n"
#endif
"          -m  = calculate and display MD5 signature; verify if lossless\n"
"          --no-utf8-convert = leave tag items in UTF-8 on extract or display\n"
"          -q  = quiet (keep console output to a minimum)\n"
#if !defined (WIN32)
"          -o FILENAME | PATH = specify output filename or path\n"
#endif
"          -r  = force raw audio decode (results in .raw extension)\n"
"          -s  = display summary information only to stdout (no audio decode)\n"
"          -ss = display super summary (including tags) to stdout (no decode)\n"
"          --skip=[sample|hh:mm:ss.ss] = start decoding at specified sample/time\n"
"          -t  = copy input file's time stamp to output file(s)\n"
"          --until=[+|-][sample|hh:mm:ss.ss] = stop decoding at specified sample/time\n"
"              (specifying a '+' causes sample/time to be relative to '--skip' point;\n"
"               specifying a '-' causes sample/time to be relative to end of file)\n"
"          -v  = verify source data only (no output file created)\n"
"          -w  = regenerate .wav header (ignore RIFF data in file)\n"
"          -x \"Field\" = extract specified tag field only to stdout (no audio decode)\n"
"          -xx \"Field[=file]\" = extract specified tag field to file, optional\n"
"              filename specification can inlude following replacement codes:\n"
"                %a = audio output filename\n"
"                %t = tag field name (note: comes from data for binary tags)\n"
"                %e = extension from binary tag source file, or 'txt' for text tag\n"
"          -y  = yes to overwrite warning (use with caution!)\n\n"
" Web:     Visit www.wavpack.com for latest version and info\n";

// this global is used to indicate the special "debug" mode where extra debug messages
// are displayed and all messages are logged to the file \wavpack.log

int debug_logging_mode;

static char overwrite_all, delete_source, raw_decode, no_utf8_convert,
    summary, ignore_wvc, quiet_mode, calc_md5, copy_time, blind_decode, wav_decode;

static int num_files, file_index, outbuf_k;

static struct sample_time_index {
    int value_is_time, value_is_relative, value_is_valid;
    double value;
} skip, until;

static char *tag_extract_stdout;    // extract single tag to stdout
static char **tag_extractions;      // extract multiple tags to named files
static int num_tag_extractions;

/////////////////////////// local function declarations ///////////////////////

static void add_tag_extraction_to_list (char *spec);
static void parse_sample_time_index (struct sample_time_index *dst, char *src);
static int unpack_file (char *infilename, char *outfilename);
static void display_progress (double file_progress);

#define NO_ERROR 0L
#define SOFT_ERROR 1
#define HARD_ERROR 2

//////////////////////////////////////////////////////////////////////////////
// The "main" function for the command-line WavPack decompressor.           //
//////////////////////////////////////////////////////////////////////////////

int main (argc, argv) int argc; char **argv;
{
#ifdef __EMX__ /* OS/2 */
    _wildcard (&argc, &argv);
#endif
    int verify_only = 0, error_count = 0, add_extension = 0, output_spec = 0, ask_help = 0, c_count = 0, x_count = 0;
    char outpath, **matches = NULL, *outfilename = NULL;
    int result;

#if defined(WIN32)
    struct _finddata_t _finddata_t;
    char selfname [MAX_PATH];

    if (GetModuleFileName (NULL, selfname, sizeof (selfname)) && filespec_name (selfname) &&
        _strupr (filespec_name (selfname)) && strstr (filespec_name (selfname), "DEBUG")) {
            char **argv_t = argv;
            int argc_t = argc;

            debug_logging_mode = TRUE;

            while (--argc_t)
                error_line ("arg %d: %s", argc - argc_t, *++argv_t);
    }
#else
    if (filespec_name (*argv))
        if (strstr (filespec_name (*argv), "ebug") || strstr (filespec_name (*argv), "DEBUG")) {
            char **argv_t = argv;
            int argc_t = argc;

            debug_logging_mode = TRUE;

            while (--argc_t)
                error_line ("arg %d: %s", argc - argc_t, *++argv_t);
    }
#endif

    // loop through command-line arguments

    while (--argc) {
        if (**++argv == '-' && (*argv)[1] == '-' && (*argv)[2]) {
            char *long_option = *argv + 2, *long_param = long_option;

            while (*long_param)
                if (*long_param++ == '=')
                    break;

            if (!strcmp (long_option, "help"))                          // --help
                ask_help = 1;
            else if (!strcmp (long_option, "no-utf8-convert"))          // --no-utf8-convert
                no_utf8_convert = 1;
            else if (!strncmp (long_option, "skip", 4)) {               // --skip
                parse_sample_time_index (&skip, long_param);

                if (!skip.value_is_valid || skip.value_is_relative) {
                    error_line ("invalid --skip parameter!");
                    ++error_count;
                }
            }
            else if (!strncmp (long_option, "until", 5)) {              // --until
                parse_sample_time_index (&until, long_param);

                if (!until.value_is_valid) {
                    error_line ("invalid --until parameter!");
                    ++error_count;
                }
            }
            else {
                error_line ("unknown option: %s !", long_option);
                ++error_count;
            }
        }
#if defined (WIN32)
        else if ((**argv == '-' || **argv == '/') && (*argv)[1])
#else
        else if ((**argv == '-') && (*argv)[1])
#endif
            while (*++*argv)
                switch (**argv) {
                    case 'Y': case 'y':
                        overwrite_all = 1;
                        break;

                    case 'C': case 'c':
                        if (++c_count == 2) {
                            add_tag_extraction_to_list ("cuesheet=%a.cue");
                            c_count = 0;
                        }

                        break;

                    case 'D': case 'd':
                        delete_source = 1;
                        break;

#if defined (WIN32)
                    case 'L': case 'l':
                        SetPriorityClass (GetCurrentProcess(), IDLE_PRIORITY_CLASS);
                        break;
#elif defined (__OS2__)
                    case 'L': case 'l':
                        DosSetPriority (0, PRTYC_IDLETIME, 0, 0);
                        break;
#endif
#if defined (WIN32)
                    case 'O': case 'o':  // ignore -o in Windows to be Linux compatible
                        break;
#else
                    case 'O': case 'o':
                        output_spec = 1;
                        break;
#endif
                    case 'T': case 't':
                        copy_time = 1;
                        break;

                    case 'V': case 'v':
                        verify_only = 1;
                        break;

                    case 'S': case 's':
                        ++summary;
                        break;

                    case 'K': case 'k':
                        outbuf_k = strtol (++*argv, argv, 10);
                        --*argv;
                        break;

                    case 'M': case 'm':
                        calc_md5 = 1;
                        break;

                    case 'B': case 'b':
                        blind_decode = 1;
                        break;

                    case 'R': case 'r':
                        raw_decode = 1;
                        break;

                    case 'W': case 'w':
                        wav_decode = 1;
                        break;

                    case 'Q': case 'q':
                        quiet_mode = 1;
                        break;

                    case 'X': case 'x':
                        if (++x_count == 3) {
                            error_line ("illegal option: %s !", *argv);
                            ++error_count;
                            x_count = 0;
                        }

                        break;

                    case 'I': case 'i':
                        ignore_wvc = 1;
                        break;

                    default:
                        error_line ("illegal option: %c !", **argv);
                        ++error_count;
                }
        else {
            if (x_count) {
                if (x_count == 1) {
                    if (tag_extract_stdout) {
                        error_line ("can't extract more than 1 tag item to stdout at a time!");
                        ++error_count;
                    }
                    else
                        tag_extract_stdout = *argv;
                }
                else if (x_count == 2)
                    add_tag_extraction_to_list (*argv);

                x_count = 0;
            }
#if defined (WIN32)
            else if (!num_files) {
                matches = realloc (matches, (num_files + 1) * sizeof (*matches));
                matches [num_files] = malloc (strlen (*argv) + 10);
                strcpy (matches [num_files], *argv);

                if (*(matches [num_files]) != '-' && *(matches [num_files]) != '@' &&
                    !filespec_ext (matches [num_files]))
                        strcat (matches [num_files], ".wv");

                num_files++;
            }
            else if (!outfilename) {
                outfilename = malloc (strlen (*argv) + PATH_MAX);
                strcpy (outfilename, *argv);
            }
            else {
                error_line ("extra unknown argument: %s !", *argv);
                ++error_count;
            }
#else
            else if (output_spec) {
                outfilename = malloc (strlen (*argv) + PATH_MAX);
                strcpy (outfilename, *argv);
                output_spec = 0;
            }
            else {
                matches = realloc (matches, (num_files + 1) * sizeof (*matches));
                matches [num_files] = malloc (strlen (*argv) + 10);
                strcpy (matches [num_files], *argv);

                if (*(matches [num_files]) != '-' && *(matches [num_files]) != '@' &&
                    !filespec_ext (matches [num_files]))
                        strcat (matches [num_files], ".wv");

                num_files++;
            }
#endif
        }
    }

   // check for various command-line argument problems

    if (delete_source && (verify_only || skip.value_is_valid || until.value_is_valid)) {
        error_line ("can't delete in verify mode or when --skip or --until are used!");
        delete_source = 0;
    }

    if (raw_decode && wav_decode) {
        error_line ("-r (raw decode) and -w (wav header) modes are incompatible!");
        ++error_count;
    }

    if (verify_only && outfilename) {
        error_line ("outfile specification and verify mode are incompatible!");
        ++error_count;
    }

    if (c_count == 1) {
        if (tag_extract_stdout) {
            error_line ("can't extract more than 1 tag item to stdout at a time!");
            error_count++;
        }
        else
            tag_extract_stdout = "cuesheet";
    }

    if (tag_extract_stdout && (num_tag_extractions || outfilename || verify_only || delete_source || wav_decode || raw_decode)) {
        error_line ("can't extract a tag to stdout and do anything else!");
        ++error_count;
    }

    if ((tag_extract_stdout || num_tag_extractions) && outfilename && *outfilename == '-') {
        error_line ("can't extract tags when unpacking audio to stdout!");
        ++error_count;
    }

    if (!quiet_mode && !error_count)
        fprintf (stderr, sign_on, VERSION_OS, WavpackGetLibraryVersionString ());

    if (!num_files || ask_help) {
        printf ("%s", usage);
        return 1;
    }

    if (error_count)
        return 1;

    setup_break ();

    for (file_index = 0; file_index < num_files; ++file_index) {
        char *infilename = matches [file_index];

        // If the single infile specification begins with a '@', then it
        // actually points to a file that contains the names of the files
        // to be converted. This was included for use by Wim Speekenbrink's
        // frontends, but could be used for other purposes.

        if (*infilename == '@') {
            FILE *list = fopen (infilename+1, "rt");
            int di, c;

            for (di = file_index; di < num_files - 1; di++)
                matches [di] = matches [di + 1];

            file_index--;
            num_files--;

            if (list == NULL) {
                error_line ("file %s not found!", infilename+1);
                free (infilename);
                return 1;
            }

            while ((c = getc (list)) != EOF) {

                while (c == '\n')
                    c = getc (list);

                if (c != EOF) {
                    char *fname = malloc (PATH_MAX);
                    int ci = 0;

                    do
                        fname [ci++] = c;
                    while ((c = getc (list)) != '\n' && c != EOF && ci < PATH_MAX);

                    fname [ci++] = '\0';
                    fname = realloc (fname, ci);
                    matches = realloc (matches, ++num_files * sizeof (*matches));

                    for (di = num_files - 1; di > file_index + 1; di--)
                        matches [di] = matches [di - 1];

                    matches [++file_index] = fname;
                }
            }

            fclose (list);
            free (infilename);
        }
#if defined (WIN32)
        else if (filespec_wild (infilename)) {
            FILE *list = fopen (infilename+1, "rt");
            intptr_t file;
            int di;

            for (di = file_index; di < num_files - 1; di++)
                matches [di] = matches [di + 1];

            file_index--;
            num_files--;

            if ((file = _findfirst (infilename, &_finddata_t)) != (intptr_t) -1) {
                do {
                    if (!(_finddata_t.attrib & _A_SUBDIR)) {
                        matches = realloc (matches, ++num_files * sizeof (*matches));

                        for (di = num_files - 1; di > file_index + 1; di--)
                            matches [di] = matches [di - 1];

                        matches [++file_index] = malloc (strlen (infilename) + strlen (_finddata_t.name) + 10);
                        strcpy (matches [file_index], infilename);
                        *filespec_name (matches [file_index]) = '\0';
                        strcat (matches [file_index], _finddata_t.name);
                    }
                } while (_findnext (file, &_finddata_t) == 0);

                _findclose (file);
            }

            free (infilename);
        }
#endif
    }

    // If the outfile specification begins with a '@', then it actually points
    // to a file that contains the output specification. This was included for
    // use by Wim Speekenbrink's frontends because certain filenames could not
    // be passed on the command-line, but could be used for other purposes.

    if (outfilename && outfilename [0] == '@') {
        FILE *list = fopen (outfilename+1, "rt");
        int c;

        if (list == NULL) {
            error_line ("file %s not found!", outfilename+1);
            free(outfilename);
            return 1;
        }

        while ((c = getc (list)) == '\n');

        if (c != EOF) {
            int ci = 0;

            do
                outfilename [ci++] = c;
            while ((c = getc (list)) != '\n' && c != EOF && ci < PATH_MAX);

            outfilename [ci] = '\0';
        }
        else {
            error_line ("output spec file is empty!");
            free(outfilename);
            fclose (list);
            return 1;
        }

        fclose (list);
    }

    // if we found any files to process, this is where we start

    if (num_files) {
        if (outfilename && *outfilename != '-') {
            outpath = (filespec_path (outfilename) != NULL);

            if (num_files > 1 && !outpath) {
                error_line ("%s is not a valid output path", outfilename);
                free (outfilename);
                return 1;
            }
        }
        else
            outpath = 0;

        add_extension = !outfilename || outpath || !filespec_ext (outfilename);

        // loop through and process files in list

        for (file_index = 0; file_index < num_files; ++file_index) {
            if (check_break ())
                break;

            // generate output filename

            if (outpath) {
                strcat (outfilename, filespec_name (matches [file_index]));

                if (filespec_ext (outfilename))
                    *filespec_ext (outfilename) = '\0';
            }
            else if (!outfilename) {
                outfilename = malloc (strlen (matches [file_index]) + 10);
                strcpy (outfilename, matches [file_index]);

                if (filespec_ext (outfilename))
                    *filespec_ext (outfilename) = '\0';
            }

            if (outfilename && *outfilename != '-' && add_extension)
                strcat (outfilename, raw_decode ? ".raw" : ".wav");

            if (num_files > 1)
                fprintf (stderr, "\n%s:\n", matches [file_index]);

            result = unpack_file (matches [file_index], verify_only ? NULL : outfilename);

            if (result != NO_ERROR)
                ++error_count;

            if (result == HARD_ERROR)
                break;

            // clean up in preparation for potentially another file

            if (outpath)
                *filespec_name (outfilename) = '\0';
            else if (*outfilename != '-') {
                free (outfilename);
                outfilename = NULL;
            }

            free (matches [file_index]);
        }

        if (num_files > 1) {
            if (error_count)
                fprintf (stderr, "\n **** warning: errors occurred in %d of %d files! ****\n", error_count, num_files);
            else if (!quiet_mode)
                fprintf (stderr, "\n **** %d files successfully processed ****\n", num_files);
        }

        free (matches);
    }
    else {
        error_line ("nothing to do!");
        ++error_count;
    }

    if (outfilename)
        free (outfilename);

#ifdef DEBUG_ALLOC
    error_line ("malloc_count = %d", dump_alloc ());
#endif

    return error_count ? 1 : 0;
}

// Parse the parameter of the --skip and --until commands, which are of the form:
//   [+|-] [samples | hh:mm:ss.ss]
// The value is returned in a double (in the "dst" struct) as either samples or
// seconds (if a time is specified). If sample, the value must be an integer.

static void parse_sample_time_index (struct sample_time_index *dst, char *src)
{
    int colons = 0;
    double temp;

    memset (dst, 0, sizeof (*dst));

    if (*src == '+' || *src == '-')
        dst->value_is_relative = (*src++ == '+') ? 1 : -1;

    while (*src)
        if (*src == ':') {
            if (++colons == 3)
                return;

            src++;
            dst->value_is_time = 1;
            dst->value *= 60.0;
            continue;
        }
        else if (*src == '.' || isdigit (*src)) {
            temp = strtod (src, &src);

            if (temp < 0.0 || (dst->value_is_time && temp >= 60.0) ||
                (!dst->value_is_time && temp != floor (temp)))
                    return;

            dst->value += temp;
        }
        else
            return;

    dst->value_is_valid = 1;
}

// Unpack the specified WavPack input file into the specified output file name.
// This function uses the library routines provided in wputils.c to do all
// unpacking. This function takes care of reformatting the data (which is
// returned in native-endian longs) to the standard little-endian format. This
// function also handles optionally calculating and displaying the MD5 sum of
// the resulting audio data and verifying the sum if a sum was stored in the
// source and lossless compression is used.

static int do_tag_extractions (WavpackContext *wpc, char *outfilename);
static unsigned char *format_samples (int bps, unsigned char *dst, int32_t *src, uint32_t samcnt);
static void dump_summary (WavpackContext *wpc, char *name, FILE *dst);
static int write_riff_header (FILE *outfile, WavpackContext *wpc, uint32_t total_samples);
static int dump_tag_item_to_file (WavpackContext *wpc, const char *tag_item, FILE *dst, char *fn);

static int unpack_file (char *infilename, char *outfilename)
{
    int result = NO_ERROR, md5_diff = FALSE, created_riff_header = FALSE;
    int open_flags = 0, bytes_per_sample, num_channels, wvc_mode, bps;
    uint32_t output_buffer_size = 0, bcount, total_unpacked_samples = 0;
    uint32_t skip_sample_index = 0, until_samples_total = 0;
    unsigned char *output_buffer = NULL, *output_pointer = NULL;
    double dtime, progress = -1.0;
    MD5_CTX md5_context;
    WavpackContext *wpc;
    int32_t *temp_buffer;
    char error [80];
    FILE *outfile;

#if defined(WIN32)
    struct _timeb time1, time2;
#else
    struct timeval time1, time2;
    struct timezone timez;
#endif

    // use library to open WavPack file

    if ((outfilename && !raw_decode && !blind_decode && !wav_decode &&
        !skip.value_is_valid && !until.value_is_valid) || summary > 1)
            open_flags |= OPEN_WRAPPER;

    if (blind_decode)
        open_flags |= OPEN_STREAMING;

    if (!ignore_wvc)
        open_flags |= OPEN_WVC;

    if (summary > 1 || num_tag_extractions || tag_extract_stdout)
        open_flags |= OPEN_TAGS;

    wpc = WavpackOpenFileInput (infilename, error, open_flags, 0);

    if (!wpc) {
        error_line (error);
        return SOFT_ERROR;
    }

    if (calc_md5)
        MD5Init (&md5_context);

    wvc_mode = WavpackGetMode (wpc) & MODE_WVC;
    num_channels = WavpackGetNumChannels (wpc);
    bps = WavpackGetBytesPerSample (wpc);
    bytes_per_sample = num_channels * bps;

    if (skip.value_is_valid) {
        if (skip.value_is_time)
            skip_sample_index = (uint32_t) (skip.value * WavpackGetSampleRate (wpc));
        else
            skip_sample_index = (uint32_t) skip.value;

        if (skip_sample_index && !WavpackSeekSample (wpc, skip_sample_index)) {
            error_line ("can't seek to specified --skip point!");
            WavpackCloseFile (wpc);
            return SOFT_ERROR;
        }

        if (WavpackGetNumSamples (wpc) != (uint32_t) -1)
            until_samples_total = WavpackGetNumSamples (wpc) - skip_sample_index;
    }

    if (until.value_is_valid) {
        if (until.value_is_time)
            until.value *= WavpackGetSampleRate (wpc);

        if (until.value_is_relative == -1) {
            if (WavpackGetNumSamples (wpc) == (uint32_t) -1) {
                error_line ("can't use negative relative --until command with files of unknown length!");
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
            }

            if ((uint32_t) until.value + skip_sample_index < WavpackGetNumSamples (wpc))
                until_samples_total = WavpackGetNumSamples (wpc) - (uint32_t) until.value - skip_sample_index;
            else
                until_samples_total = 0;
        }
        else {
            if (until.value_is_relative == 1)
                until_samples_total = (uint32_t) until.value;
            else if ((uint32_t) until.value > skip_sample_index)
                until_samples_total = (uint32_t) until.value - skip_sample_index;
            else
                until_samples_total = 0;

            if (WavpackGetNumSamples (wpc) != (uint32_t) -1 &&
                skip_sample_index + until_samples_total > WavpackGetNumSamples (wpc))
                    until_samples_total = WavpackGetNumSamples (wpc) - skip_sample_index;
        }

        if (!until_samples_total) {
            error_line ("--until command results in no samples to decode!");
            WavpackCloseFile (wpc);
            return SOFT_ERROR;
        }
    }

    if (summary) {
        dump_summary (wpc, infilename, stdout);
        WavpackCloseFile (wpc);
        return NO_ERROR;
    }

    if (tag_extract_stdout) {
        if (!dump_tag_item_to_file (wpc, tag_extract_stdout, stdout, NULL)) {
            error_line ("tag \"%s\" not found!", tag_extract_stdout);
            WavpackCloseFile (wpc);
            return SOFT_ERROR;
        }

        WavpackCloseFile (wpc);
        return NO_ERROR;
    }
    else if (num_tag_extractions && outfilename && *outfilename != '-' && filespec_name (outfilename)) {
        result = do_tag_extractions (wpc, outfilename);

        if (result != NO_ERROR) {
            WavpackCloseFile (wpc);
            return result;
        }
    }

    if (outfilename) {
        if (*outfilename != '-') {

            // check the output file for overwrite warning required

            if (!overwrite_all && (outfile = fopen (outfilename, "rb")) != NULL) {
                DoCloseHandle (outfile);
                fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (outfilename));
#if defined(WIN32)
                SetConsoleTitle ("overwrite?");
#endif
                switch (yna ()) {

                    case 'n':
                        result = SOFT_ERROR;
                        break;

                    case 'a':
                        overwrite_all = 1;
                }

                if (result != NO_ERROR) {
                    WavpackCloseFile (wpc);
                    return result;
                }
            }

            // open output file for writing

            if ((outfile = fopen (outfilename, "wb")) == NULL) {
                error_line ("can't create file %s!", FN_FIT (outfilename));
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
            }
            else if (!quiet_mode)
                fprintf (stderr, "restoring %s,", FN_FIT (outfilename));
        }
        else {  // come here to open stdout as destination

            outfile = stdout;
#if defined(WIN32)
            _setmode (fileno (stdout), O_BINARY);
#endif
#if defined(__OS2__)
            setmode (fileno (stdout), O_BINARY);
#endif

            if (!quiet_mode)
                fprintf (stderr, "unpacking %s%s to stdout,", *infilename == '-' ?
                    "stdin" : FN_FIT (infilename), wvc_mode ? " (+.wvc)" : "");
        }

        if (outbuf_k)
            output_buffer_size = outbuf_k * 1024;
        else
            output_buffer_size = 1024 * 256;

        output_pointer = output_buffer = malloc (output_buffer_size);
    }
    else {      // in verify only mode we don't worry about headers
        outfile = NULL;

        if (!quiet_mode)
            fprintf (stderr, "verifying %s%s,", *infilename == '-' ? "stdin" :
                FN_FIT (infilename), wvc_mode ? " (+.wvc)" : "");
    }

#if defined(WIN32)
    _ftime (&time1);
#else
    gettimeofday(&time1,&timez);
#endif

    if (outfile && !raw_decode) {
        if (until_samples_total) {
            if (!write_riff_header (outfile, wpc, until_samples_total)) {
                DoTruncateFile (outfile);
                result = HARD_ERROR;
            }
            else
                created_riff_header = TRUE;
        }
        else if (WavpackGetWrapperBytes (wpc)) {
            if (!DoWriteFile (outfile, WavpackGetWrapperData (wpc), WavpackGetWrapperBytes (wpc), &bcount) ||
                bcount != WavpackGetWrapperBytes (wpc)) {
                    error_line ("can't write .WAV data, disk probably full!");
                    DoTruncateFile (outfile);
                    result = HARD_ERROR;
            }

            WavpackFreeWrapper (wpc);
        }
        else if (!write_riff_header (outfile, wpc, WavpackGetNumSamples (wpc))) {
            DoTruncateFile (outfile);
            result = HARD_ERROR;
        }
        else
            created_riff_header = TRUE;
    }

    temp_buffer = malloc (4096L * num_channels * 4);

    while (result == NO_ERROR) {
        uint32_t samples_to_unpack, samples_unpacked;

        if (output_buffer) {
            samples_to_unpack = (output_buffer_size - (output_pointer - output_buffer)) / bytes_per_sample;

            if (samples_to_unpack > 4096)
                samples_to_unpack = 4096;
        }
        else
            samples_to_unpack = 4096;

        if (until_samples_total && samples_to_unpack > until_samples_total - total_unpacked_samples)
            samples_to_unpack = until_samples_total - total_unpacked_samples;

        samples_unpacked = WavpackUnpackSamples (wpc, temp_buffer, samples_to_unpack);
        total_unpacked_samples += samples_unpacked;

        if (output_buffer) {
            if (samples_unpacked)
                output_pointer = format_samples (bps, output_pointer, temp_buffer, samples_unpacked * num_channels);

            if (!samples_unpacked || (output_buffer_size - (output_pointer - output_buffer)) < (uint32_t) bytes_per_sample) {
                if (!DoWriteFile (outfile, output_buffer, (uint32_t)(output_pointer - output_buffer), &bcount) ||
                    bcount != output_pointer - output_buffer) {
                        error_line ("can't write .WAV data, disk probably full!");
                        DoTruncateFile (outfile);
                        result = HARD_ERROR;
                        break;
                }

                output_pointer = output_buffer;
            }
        }

        if (calc_md5 && samples_unpacked) {
            format_samples (bps, (unsigned char *) temp_buffer, temp_buffer, samples_unpacked * num_channels);
            MD5Update (&md5_context, (unsigned char *) temp_buffer, bps * samples_unpacked * num_channels);
        }

        if (!samples_unpacked)
            break;

        if (check_break ()) {
            fprintf (stderr, "^C\n");
            DoTruncateFile (outfile);
            result = SOFT_ERROR;
            break;
        }

        if (WavpackGetProgress (wpc) != -1.0 &&
            progress != floor (WavpackGetProgress (wpc) * 100.0 + 0.5)) {
                int nobs = progress == -1.0;

                progress = WavpackGetProgress (wpc);
                display_progress (progress);
                progress = floor (progress * 100.0 + 0.5);

                if (!quiet_mode)
                    fprintf (stderr, "%s%3d%% done...",
                        nobs ? " " : "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
        }
    }

    if (output_buffer)
        free (output_buffer);

    if (!check_break () && calc_md5) {
        char md5_string1 [] = "00000000000000000000000000000000";
        char md5_string2 [] = "00000000000000000000000000000000";
        unsigned char md5_original [16], md5_unpacked [16];
        int i;

        MD5Final (md5_unpacked, &md5_context);

        if (WavpackGetMD5Sum (wpc, md5_original)) {

            for (i = 0; i < 16; ++i)
                sprintf (md5_string1 + (i * 2), "%02x", md5_original [i]);

            error_line ("original md5:  %s", md5_string1);

            if (memcmp (md5_unpacked, md5_original, 16))
                md5_diff = TRUE;
        }

        for (i = 0; i < 16; ++i)
            sprintf (md5_string2 + (i * 2), "%02x", md5_unpacked [i]);

        error_line ("unpacked md5:  %s", md5_string2);
    }

    while (!created_riff_header && WavpackGetWrapperBytes (wpc)) {
        if (outfile && result == NO_ERROR &&
            (!DoWriteFile (outfile, WavpackGetWrapperData (wpc), WavpackGetWrapperBytes (wpc), &bcount) ||
            bcount != WavpackGetWrapperBytes (wpc))) {
                error_line ("can't write .WAV data, disk probably full!");
                DoTruncateFile (outfile);
                result = HARD_ERROR;
        }

        WavpackFreeWrapper (wpc);
        WavpackUnpackSamples (wpc, temp_buffer, 1); // perhaps there's more RIFF info...
    }

    free (temp_buffer);

    if (result == NO_ERROR && outfile && created_riff_header &&
        (WavpackGetNumSamples (wpc) == (uint32_t) -1 ||
         (until_samples_total ? until_samples_total : WavpackGetNumSamples (wpc)) != total_unpacked_samples)) {
            if (*outfilename == '-' || DoSetFilePositionAbsolute (outfile, 0))
                error_line ("can't update RIFF header with actual size");
            else if (!write_riff_header (outfile, wpc, total_unpacked_samples)) {
                DoTruncateFile (outfile);
                result = HARD_ERROR;
            }
    }

    // if we are not just in verify only mode, grab the size of the output
    // file and close the file

    if (outfile != NULL) {
        int64_t outfile_length;

        fflush (outfile);
        outfile_length = DoGetFileSize (outfile);

        if (!DoCloseHandle (outfile)) {
            error_line ("can't close file %s!", FN_FIT (outfilename));
            result = SOFT_ERROR;
        }

        if (outfilename && *outfilename != '-' && !outfile_length)
            DoDeleteFile (outfilename);
    }

    if (result == NO_ERROR && copy_time && outfilename &&
        !copy_timestamp (infilename, outfilename))
            error_line ("failure copying time stamp!");

    if (result == NO_ERROR) {
        if (!until_samples_total && WavpackGetNumSamples (wpc) != (uint32_t) -1) {
            if (total_unpacked_samples < WavpackGetNumSamples (wpc)) {
                error_line ("file is missing %u samples!",
                    WavpackGetNumSamples (wpc) - total_unpacked_samples);
                result = SOFT_ERROR;
            }
            else if (total_unpacked_samples > WavpackGetNumSamples (wpc)) {
                error_line ("file has %u extra samples!",
                    total_unpacked_samples - WavpackGetNumSamples (wpc));
                result = SOFT_ERROR;
            }
        }

        if (WavpackGetNumErrors (wpc)) {
            error_line ("missing data or crc errors detected in %d block(s)!", WavpackGetNumErrors (wpc));
            result = SOFT_ERROR;
        }
    }

    if (result == NO_ERROR && md5_diff && (WavpackGetMode (wpc) & MODE_LOSSLESS) && !until_samples_total) {
        error_line ("MD5 signatures should match, but do not!");
        result = SOFT_ERROR;
    }

    // Compute and display the time consumed along with some other details of
    // the unpacking operation (assuming there was no error).

#if defined(WIN32)
    _ftime (&time2);
    dtime = time2.time + time2.millitm / 1000.0;
    dtime -= time1.time + time1.millitm / 1000.0;
#else
    gettimeofday(&time2,&timez);
    dtime = time2.tv_sec + time2.tv_usec / 1000000.0;
    dtime -= time1.tv_sec + time1.tv_usec / 1000000.0;
#endif

    if (result == NO_ERROR && !quiet_mode) {
        char *file, *fext, *oper, *cmode, cratio [16] = "";

        if (outfilename && *outfilename != '-') {
            file = FN_FIT (outfilename);
            fext = "";
            oper = "restored";
        }
        else {
            file = (*infilename == '-') ? "stdin" : FN_FIT (infilename);
            fext = wvc_mode ? " (+.wvc)" : "";
            oper = outfilename ? "unpacked" : "verified";
        }

        if (WavpackGetMode (wpc) & MODE_LOSSLESS) {
            cmode = "lossless";

            if (WavpackGetRatio (wpc) != 0.0)
                sprintf (cratio, ", %.2f%%", 100.0 - WavpackGetRatio (wpc) * 100.0);
        }
        else {
            cmode = "lossy";

            if (WavpackGetAverageBitrate (wpc, TRUE) != 0.0)
                sprintf (cratio, ", %d kbps", (int) (WavpackGetAverageBitrate (wpc, TRUE) / 1000.0));
        }

        error_line ("%s %s%s in %.2f secs (%s%s)", oper, file, fext, dtime, cmode, cratio);
    }

    WavpackCloseFile (wpc);

    if (result == NO_ERROR && delete_source) {
        int res = DoDeleteFile (infilename);

        if (!quiet_mode || !res)
            error_line ("%s source file %s", res ?
                "deleted" : "can't delete", infilename);

        if (wvc_mode) {
            char in2filename [PATH_MAX];

            strcpy (in2filename, infilename);
            strcat (in2filename, "c");
            res = DoDeleteFile (in2filename);

            if (!quiet_mode || !res)
                error_line ("%s source file %s", res ?
                    "deleted" : "can't delete", in2filename);
        }
    }

    return result;
}

static void add_tag_extraction_to_list (char *spec)
{
    tag_extractions = realloc (tag_extractions, (num_tag_extractions + 1) * sizeof (*tag_extractions));
    tag_extractions [num_tag_extractions] = malloc (strlen (spec) + 10);
    strcpy (tag_extractions [num_tag_extractions], spec);
    num_tag_extractions++;
}

static int do_tag_extractions (WavpackContext *wpc, char *outfilename)
{
    int result = NO_ERROR, i;
    FILE *outfile;

    for (i = 0; result == NO_ERROR && i < num_tag_extractions; ++i) {
        char *extraction_spec = strdup (tag_extractions [i]);
        char *output_spec = strchr (extraction_spec, '=');
        char tag_filename [256];

        if (output_spec && output_spec > extraction_spec && strlen (output_spec) > 1)
            *output_spec++ = 0;

        if (dump_tag_item_to_file (wpc, extraction_spec, NULL, tag_filename)) {
            int max_length = (int) strlen (outfilename) + (int) strlen (tag_filename) + 10;
            char *full_filename;

            if (output_spec)
                max_length += (int) strlen (output_spec) + 256;

            full_filename = malloc (max_length * 2 + 1);
            strcpy (full_filename, outfilename);

            if (output_spec) {
                char *dst = filespec_name (full_filename);

                while (*output_spec && dst - full_filename < max_length) {
                    if (*output_spec == '%') {
                        switch (*++output_spec) {
                            case 'a':                           // audio filename
                                strcpy (dst, filespec_name (outfilename));

                                if (filespec_ext (dst))         // get rid of any extension
                                    dst = filespec_ext (dst);
                                else
                                    dst += strlen (dst);

                                output_spec++;
                                break;

                            case 't':                           // tag field name
                                strcpy (dst, tag_filename);

                                if (filespec_ext (dst))         // get rid of any extension
                                    dst = filespec_ext (dst);
                                else
                                    dst += strlen (dst);

                                output_spec++;
                                break;

                            case 'e':                           // default extension
                                if (filespec_ext (tag_filename)) {
                                    strcpy (dst, filespec_ext (tag_filename) + 1);
                                    dst += strlen (dst);
                                }

                                output_spec++;
                                break;

                            default:
                                *dst++ = '%';
                        }
                    }
                    else
                        *dst++ = *output_spec++;
                }

                *dst = 0;
            }
            else
                strcpy (filespec_name (full_filename), tag_filename);

            if (!overwrite_all && (outfile = fopen (full_filename, "r")) != NULL) {
                DoCloseHandle (outfile);
                fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (full_filename));
#if defined(WIN32)
                SetConsoleTitle ("overwrite?");
#endif
                switch (yna ()) {

                    case 'n':
                        *full_filename = 0;
                        break;

                    case 'a':
                        overwrite_all = 1;
                }
            }

            // open output file for writing

            if (*full_filename) {
                if ((outfile = fopen (full_filename, "w")) == NULL) {
                    error_line ("can't create file %s!", FN_FIT (full_filename));
                    result = SOFT_ERROR;
                }
                else {
                    dump_tag_item_to_file (wpc, extraction_spec, outfile, NULL);

                    if (!DoCloseHandle (outfile)) {
                        error_line ("can't close file %s!", FN_FIT (full_filename));
                        result = SOFT_ERROR;
                    }
                    else if (!quiet_mode)
                        error_line ("extracted tag \"%s\" to file %s", extraction_spec, FN_FIT (full_filename));
                }
            }

            free (full_filename);
        }

        free (extraction_spec);
    }

    return result;
}

// Reformat samples from longs in processor's native endian mode to
// little-endian data with (possibly) less than 4 bytes / sample.

static unsigned char *format_samples (int bps, unsigned char *dst, int32_t *src, uint32_t samcnt)
{
    int32_t temp;

    switch (bps) {

        case 1:
            while (samcnt--)
                *dst++ = *src++ + 128;

            break;

        case 2:
            while (samcnt--) {
                *dst++ = (unsigned char) (temp = *src++);
                *dst++ = (unsigned char) (temp >> 8);
            }

            break;

        case 3:
            while (samcnt--) {
                *dst++ = (unsigned char) (temp = *src++);
                *dst++ = (unsigned char) (temp >> 8);
                *dst++ = (unsigned char) (temp >> 16);
            }

            break;

        case 4:
            while (samcnt--) {
                *dst++ = (unsigned char) (temp = *src++);
                *dst++ = (unsigned char) (temp >> 8);
                *dst++ = (unsigned char) (temp >> 16);
                *dst++ = (unsigned char) (temp >> 24);
            }

            break;
    }

    return dst;
}

static int write_riff_header (FILE *outfile, WavpackContext *wpc, uint32_t total_samples)
{
    RiffChunkHeader riffhdr;
    ChunkHeader datahdr, fmthdr;
    WaveHeader wavhdr;
    uint32_t bcount;

    uint32_t total_data_bytes;
    int num_channels = WavpackGetNumChannels (wpc);
    int32_t channel_mask = WavpackGetChannelMask (wpc);
    int32_t sample_rate = WavpackGetSampleRate (wpc);
    int bytes_per_sample = WavpackGetBytesPerSample (wpc);
    int bits_per_sample = WavpackGetBitsPerSample (wpc);
    int format = WavpackGetFloatNormExp (wpc) ? 3 : 1;
    int wavhdrsize = 16;

    if (format == 3 && WavpackGetFloatNormExp (wpc) != 127) {
        error_line ("can't create valid RIFF wav header for non-normalized floating data!");
        return FALSE;
    }

    if (total_samples == (uint32_t) -1)
        total_samples = 0x7ffff000 / (bytes_per_sample * num_channels);

    total_data_bytes = total_samples * bytes_per_sample * num_channels;

    CLEAR (wavhdr);

    wavhdr.FormatTag = format;
    wavhdr.NumChannels = num_channels;
    wavhdr.SampleRate = sample_rate;
    wavhdr.BytesPerSecond = sample_rate * num_channels * bytes_per_sample;
    wavhdr.BlockAlign = bytes_per_sample * num_channels;
    wavhdr.BitsPerSample = bits_per_sample;

    if (num_channels > 2 || channel_mask != 0x5 - num_channels) {
        wavhdrsize = sizeof (wavhdr);
        wavhdr.cbSize = 22;
        wavhdr.ValidBitsPerSample = bits_per_sample;
        wavhdr.SubFormat = format;
        wavhdr.ChannelMask = channel_mask;
        wavhdr.FormatTag = 0xfffe;
        wavhdr.BitsPerSample = bytes_per_sample * 8;
        wavhdr.GUID [4] = 0x10;
        wavhdr.GUID [6] = 0x80;
        wavhdr.GUID [9] = 0xaa;
        wavhdr.GUID [11] = 0x38;
        wavhdr.GUID [12] = 0x9b;
        wavhdr.GUID [13] = 0x71;
    }

    strncpy (riffhdr.ckID, "RIFF", sizeof (riffhdr.ckID));
    strncpy (riffhdr.formType, "WAVE", sizeof (riffhdr.formType));
    riffhdr.ckSize = sizeof (riffhdr) + wavhdrsize + sizeof (datahdr) + total_data_bytes;
    strncpy (fmthdr.ckID, "fmt ", sizeof (fmthdr.ckID));
    fmthdr.ckSize = wavhdrsize;

    strncpy (datahdr.ckID, "data", sizeof (datahdr.ckID));
    datahdr.ckSize = total_data_bytes;

    // write the RIFF chunks up to just before the data starts

    WavpackNativeToLittleEndian (&riffhdr, ChunkHeaderFormat);
    WavpackNativeToLittleEndian (&fmthdr, ChunkHeaderFormat);
    WavpackNativeToLittleEndian (&wavhdr, WaveHeaderFormat);
    WavpackNativeToLittleEndian (&datahdr, ChunkHeaderFormat);

    if (!DoWriteFile (outfile, &riffhdr, sizeof (riffhdr), &bcount) || bcount != sizeof (riffhdr) ||
        !DoWriteFile (outfile, &fmthdr, sizeof (fmthdr), &bcount) || bcount != sizeof (fmthdr) ||
        !DoWriteFile (outfile, &wavhdr, wavhdrsize, &bcount) || bcount != wavhdrsize ||
        !DoWriteFile (outfile, &datahdr, sizeof (datahdr), &bcount) || bcount != sizeof (datahdr)) {
            error_line ("can't write .WAV data, disk probably full!");
            return FALSE;
    }

    return TRUE;
}

static void dump_UTF8_string (char *string, FILE *dst);
static void UTF8ToAnsi (char *string, int len);
static const char *speakers [] = {
    "FL", "FR", "FC", "LFE", "BL", "BR", "FLC", "FRC", "BC",
    "SL", "SR", "TC", "TFL", "TFC", "TFR", "TBL", "TBC", "TBR"
};

static void dump_summary (WavpackContext *wpc, char *name, FILE *dst)
{
    uint32_t channel_mask = (uint32_t) WavpackGetChannelMask (wpc);
    int num_channels = WavpackGetNumChannels (wpc);
    unsigned char md5_sum [16];
    char modes [80];

    fprintf (dst, "\n");

    if (name && *name != '-') {
        fprintf (dst, "file name:         %s%s\n", name, (WavpackGetMode (wpc) & MODE_WVC) ? " (+wvc)" : "");
        fprintf (dst, "file size:         %u bytes\n", WavpackGetFileSize (wpc));
    }

    fprintf (dst, "source:            %d-bit %s at %u Hz\n", WavpackGetBitsPerSample (wpc),
        (WavpackGetMode (wpc) & MODE_FLOAT) ? "floats" : "ints",
        WavpackGetSampleRate (wpc));

    if (!channel_mask)
        strcpy (modes, "unassigned speakers");
    else if (num_channels == 1 && channel_mask == 0x4)
        strcpy (modes, "mono");
    else if (num_channels == 2 && channel_mask == 0x3)
        strcpy (modes, "stereo");
    else {
        int cc = num_channels, si = 0;
        uint32_t cm = channel_mask;

        modes [0] = 0;

        while (cc && cm) {
            if (cm & 1) {
                strcat (modes, speakers [si]);
                if (--cc)
                    strcat (modes, ",");
            }
            cm >>= 1;
            si++;
        }

        if (cc)
            strcat (modes, "...");
    }

    fprintf (dst, "channels:          %d (%s)\n", num_channels, modes);

    if (WavpackGetNumSamples (wpc) != (uint32_t) -1) {
        double seconds = (double) WavpackGetNumSamples (wpc) / WavpackGetSampleRate (wpc);
        int minutes = (int) floor (seconds / 60.0);
        int hours = (int) floor (seconds / 3600.0);

        seconds -= minutes * 60.0;
        minutes -= (int)(hours * 60.0);

        fprintf (dst, "duration:          %d:%02d:%05.2f\n", hours, minutes, seconds);
    }

    modes [0] = 0;

    if (WavpackGetMode (wpc) & MODE_HYBRID)
        strcat (modes, "hybrid ");

    strcat (modes, (WavpackGetMode (wpc) & MODE_LOSSLESS) ? "lossless" : "lossy");

    if (WavpackGetMode (wpc) & MODE_FAST)
        strcat (modes, ", fast");
    else if (WavpackGetMode (wpc) & MODE_VERY_HIGH)
        strcat (modes, ", very high");
    else if (WavpackGetMode (wpc) & MODE_HIGH)
        strcat (modes, ", high");

    if (WavpackGetMode (wpc) & MODE_EXTRA) {
        strcat (modes, ", extra");

        if (WavpackGetMode (wpc) & MODE_XMODE) {
            char xmode[3] = "-0";

            xmode [1] = ((WavpackGetMode (wpc) & MODE_XMODE) >> 12) + '0';
            strcat (modes, xmode);
        }
    }

    if (WavpackGetMode (wpc) & MODE_SFX)
        strcat (modes, ", sfx");

    if (WavpackGetMode (wpc) & MODE_DNS)
        strcat (modes, ", dns");

    fprintf (dst, "modalities:        %s\n", modes);

    if (WavpackGetRatio (wpc) != 0.0) {
        fprintf (dst, "compression:       %.2f%%\n", 100.0 - (100 * WavpackGetRatio (wpc)));
        fprintf (dst, "ave bitrate:       %d kbps\n", (int) ((WavpackGetAverageBitrate (wpc, TRUE) + 500.0) / 1000.0));

        if (WavpackGetMode (wpc) & MODE_WVC)
            fprintf (dst, "ave lossy bitrate: %d kbps\n", (int) ((WavpackGetAverageBitrate (wpc, FALSE) + 500.0) / 1000.0));
    }

    if (WavpackGetVersion (wpc))
        fprintf (dst, "encoder version:   %d\n", WavpackGetVersion (wpc));

    if (WavpackGetMD5Sum (wpc, md5_sum)) {
        char md5_string [] = "00000000000000000000000000000000";
        int i;

        for (i = 0; i < 16; ++i)
            sprintf (md5_string + (i * 2), "%02x", md5_sum [i]);

        fprintf (dst, "original md5:      %s\n", md5_string);
    }

    if (summary > 1) {
        uint32_t header_bytes = WavpackGetWrapperBytes (wpc), trailer_bytes, i;
        unsigned char *header_data = WavpackGetWrapperData (wpc);
        char header_name [5];

        strcpy (header_name, "????");

        for (i = 0; i < 4 && i < header_bytes; ++i)
            if (header_data [i] >= 0x20 && header_data [i] <= 0x7f)
                header_name [i] = header_data [i];

        WavpackFreeWrapper (wpc);
        WavpackSeekTrailingWrapper (wpc);
        trailer_bytes = WavpackGetWrapperBytes (wpc);

        if (header_bytes && trailer_bytes)
            fprintf (dst, "file wrapper:      %d + %d bytes (%s)\n",
                header_bytes, trailer_bytes, header_name);
        else if (header_bytes)
            fprintf (dst, "file wrapper:      %d byte %s header\n",
                header_bytes, header_name);
        else if (trailer_bytes)
            fprintf (dst, "file wrapper:      %d byte trailer only\n",
                trailer_bytes);
        else
            fprintf (dst, "file wrapper:      none (raw audio)\n");
    }

    if (WavpackGetMode (wpc) & MODE_VALID_TAG) {
        int ape_tag = WavpackGetMode (wpc) & MODE_APETAG;
        int num_binary_items = WavpackGetNumBinaryTagItems (wpc);
        int num_items = WavpackGetNumTagItems (wpc), i;
        char *spaces = "                  ";

        fprintf (dst, "\n%s tag items:   %d\n", ape_tag ? "APEv2" : "ID3v1", num_items + num_binary_items);

        for (i = 0; i < num_items; ++i) {
            int item_len, value_len, j;
            char *item, *value;

            item_len = WavpackGetTagItemIndexed (wpc, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetTagItemIndexed (wpc, i, item, item_len + 1);
            value_len = WavpackGetTagItem (wpc, item, NULL, 0);
            value = malloc (value_len * 2 + 1);
            WavpackGetTagItem (wpc, item, value, value_len + 1);

            fprintf (dst, "%s:%s", item, strlen (item) < strlen (spaces) ? spaces + strlen (item) : " ");

            if (ape_tag) {
                for (j = 0; j < value_len; ++j)
                    if (!value [j])
                        value [j] = '\\';

                if (strchr (value, '\n'))
                    fprintf (dst, "%d-byte multi-line text string\n", value_len);
                else {
                    dump_UTF8_string (value, dst);
                    fprintf (dst, "\n");
                }
            }
            else
                fprintf (dst, "%s\n", value);

            free (value);
            free (item);
        }

        for (i = 0; i < num_binary_items; ++i) {
            int item_len, value_len;
            char *item, fname [256];

            item_len = WavpackGetBinaryTagItemIndexed (wpc, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetBinaryTagItemIndexed (wpc, i, item, item_len + 1);
            value_len = dump_tag_item_to_file (wpc, item, NULL, fname);
            fprintf (dst, "%s:%s", item, strlen (item) < strlen (spaces) ? spaces + strlen (item) : " ");

            if (filespec_ext (fname))
                fprintf (dst, "%d-byte binary item (%s)\n", value_len, filespec_ext (fname)+1);
            else
                fprintf (dst, "%d-byte binary item\n", value_len);

#if 0   // debug binary tag reading
            {
                char md5_string [] = "00000000000000000000000000000000";
                unsigned char md5_result [16];
                MD5_CTX md5_context;
                char *value;
                int i, j;

                MD5Init (&md5_context);
                value_len = WavpackGetBinaryTagItem (wpc, item, NULL, 0);
                value = malloc (value_len);
                value_len = WavpackGetBinaryTagItem (wpc, item, value, value_len);

                for (i = 0; i < value_len; ++i)
                    if (!value [i]) {
                        MD5Update (&md5_context, (unsigned char *) value + i + 1, value_len - i - 1);
                        MD5Final (md5_result, &md5_context);
                        for (j = 0; j < 16; ++j)
                            sprintf (md5_string + (j * 2), "%02x", md5_result [j]);
                        fprintf (dst, "    %d byte string >>%s<<\n", i, value);
                        fprintf (dst, "    %d bytes binary data >>%s<<\n", value_len - i - 1, md5_string);
                        break;
                    }

                if (i == value_len)
                    fprintf (dst, "    no NULL found in binary value (or value not readable)\n");

                free (value);
            }
#endif
            free (item);
        }
    }
}

// Dump the specified tag field to the specified stream. Both text and binary tags may be written,
// and in Windows the appropriate file mode will be set. If the tag is not found then 0 is returned,
// otherwise the length of the data is returned, and this is true even when the file pointer is NULL
// so this can be used to determine if the tag exists before further processing.
//
// The "fname" parameter can optionally be set to a character array that will accept the suggested
// filename. This is formed by the tag item name with the extension ".txt" for text fields; for
// binary fields this is supplied by convention as a NULL terminated string at the beginning of the
// data, so this is returned. The string should have 256 characters available.

static int dump_tag_item_to_file (WavpackContext *wpc, const char *tag_item, FILE *dst, char *fname)
{
    if (WavpackGetMode (wpc) & MODE_VALID_TAG) {
        if (WavpackGetTagItem (wpc, tag_item, NULL, 0)) {
            int value_len = WavpackGetTagItem (wpc, tag_item, NULL, 0);
            char *value;

            if (fname) {
                strcpy (fname, tag_item);
                strcat (fname, ".txt");
            }

            if (!value_len || !dst)
                return value_len;

#if defined(WIN32)
            _setmode (fileno (dst), O_TEXT);
#endif
#if defined(__OS2__)
            setmode (fileno (dst), O_TEXT);
#endif
            value = malloc (value_len * 2 + 1);
            WavpackGetTagItem (wpc, tag_item, value, value_len + 1);
            dump_UTF8_string (value, dst);
            free (value);
            return value_len;
        }
        else if (WavpackGetBinaryTagItem (wpc, tag_item, NULL, 0)) {
            int value_len = WavpackGetBinaryTagItem (wpc, tag_item, NULL, 0), res = 0, i;
            uint32_t bcount = 0;
            char *value;

            value = malloc (value_len);
            WavpackGetBinaryTagItem (wpc, tag_item, value, value_len);

            for (i = 0; i < value_len; ++i)
                if (!value [i]) {

                    if (dst) {
#if defined(WIN32)
                        _setmode (fileno (dst), O_BINARY);
#endif
#if defined(__OS2__)
                        setmode (fileno (dst), O_BINARY);
#endif
                        res = DoWriteFile (dst, (unsigned char *) value + i + 1, value_len - i - 1, &bcount);
                    }

                    if (fname) {
                        if (i < 256)
                            strcpy (fname, value);
                        else {
                            strcpy (fname, tag_item);
                            strcat (fname, ".bin");
                        }
                    }

                    break;
                }

            free (value);

            if (i == value_len)
                return 0;

            if (dst && (!res || bcount != value_len - i - 1))
                return 0;

            return value_len - i - 1;
        }
        else
            return 0;
    }
    else
        return 0;
}

// Dump the specified null-terminated, possibly multi-line, UTF-8 string to
// the specified stream. To make sure that this works correctly on both
// Windows and Linux, all CR characters ('\r') are removed from the stream
// and it is assumed that the output FILE will be in "text" mode (on Windows).
// Lines are processed and transmitted one at a time.

static void dump_UTF8_string (char *string, FILE *dst)
{
    while (*string) {
        char *p = string, *temp;
        int len = 0;

        while (*p) {
            if (*p != '\r')
                ++len;

            if (*p++ == '\n')
                break;
        }

        if (!len)
            return;

        p = temp = malloc (len * 2 + 1);

        while (*string) {
            if (*string != '\r')
                *p++ = *string;

            if (*string++ == '\n')
                break;
        }

        *p = 0;

        if (!no_utf8_convert)
            UTF8ToAnsi (temp, len * 2);

        fputs (temp, dst);
        free (temp);
    }
}

#if defined (WIN32)

// Convert Unicode UTF-8 string to wide format. UTF-8 string must be NULL
// terminated. Resulting wide string must be able to fit in provided space
// and will also be NULL terminated. The number of characters converted will
// be returned (not counting terminator).

static int UTF8ToWideChar (const unsigned char *pUTF8, unsigned short *pWide)
{
    int trail_bytes = 0;
    int chrcnt = 0;

    while (*pUTF8) {
        if (*pUTF8 & 0x80) {
            if (*pUTF8 & 0x40) {
                if (trail_bytes) {
                    trail_bytes = 0;
                    chrcnt++;
                }
                else {
                    char temp = *pUTF8;

                    while (temp & 0x80) {
                        trail_bytes++;
                        temp <<= 1;
                    }

                    pWide [chrcnt] = temp >> trail_bytes--;
                }
            }
            else if (trail_bytes) {
                pWide [chrcnt] = (pWide [chrcnt] << 6) | (*pUTF8 & 0x3f);

                if (!--trail_bytes)
                    chrcnt++;
            }
        }
        else
            pWide [chrcnt++] = *pUTF8;

        pUTF8++;
    }

    pWide [chrcnt] = 0;
    return chrcnt;
}

#endif

// Convert a Unicode UTF-8 format string into its Ansi equivalent. The
// conversion is done in-place so the maximum length of the string buffer must
// be specified because the string may become longer or shorter. If the
// resulting string will not fit in the specified buffer size then it is
// truncated.

static void UTF8ToAnsi (char *string, int len)
{
    int max_chars = (int) strlen (string);
#if defined (WIN32)
    unsigned short *temp = malloc ((max_chars + 1) * 2);
    int act_chars = UTF8ToWideChar (string, temp);

    while (act_chars) {
        memset (string, 0, len);

        if (WideCharToMultiByte (CP_ACP, 0, temp, act_chars, string, len - 1, NULL, NULL))
            break;
        else
            act_chars--;
    }

    if (!act_chars)
        *string = 0;
#else
    char *temp = malloc (len);
    char *outp = temp;
    char *inp = string;
    size_t insize = max_chars;
    size_t outsize = len - 1;
    int err = 0;
    char *old_locale;
    iconv_t converter;

    memset(temp, 0, len);
    old_locale = setlocale (LC_CTYPE, "");
    converter = iconv_open ("", "UTF-8");

    if (converter != (iconv_t) -1) {
        err = iconv (converter, &inp, &insize, &outp, &outsize);
        iconv_close (converter);
    }
    else
        err = -1;

    setlocale (LC_CTYPE, old_locale);

    if (err == -1) {
        free(temp);
        return;
    }

    memmove (string, temp, len);
#endif
    free (temp);
}

//////////////////////////////////////////////////////////////////////////////
// This function displays the progress status on the title bar of the DOS   //
// window that WavPack is running in. The "file_progress" argument is for   //
// the current file only and ranges from 0 - 1; this function takes into    //
// account the total number of files to generate a batch progress number.   //
//////////////////////////////////////////////////////////////////////////////

void display_progress (double file_progress)
{
    char title [40];

    file_progress = (file_index + file_progress) / num_files;
    sprintf (title, "%d%% (WvUnpack)", (int) ((file_progress * 100.0) + 0.5));
#if defined(WIN32)
    SetConsoleTitle (title);
#endif
}
