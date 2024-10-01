#include "config.h"
#include "wiretap/wtap-int.h"
#include "wiretap/file_wrappers.h"

WS_DLL_PUBLIC_DEF const gchar plugin_version[] = VCD_VERSION;
WS_DLL_PUBLIC_DEF const int plugin_want_major = VERSION_MAJOR;
WS_DLL_PUBLIC_DEF const int plugin_want_minor = VERSION_MINOR;

void wtap_register_vcd(void);


static gboolean vcd_read(wtap* wth, wtap_rec* rec, Buffer* buf,
                         int* err, gchar** err_info,
                         gint64* data_offset);
static gboolean vcd_seek_read(wtap* wth, gint64 seek_off,
                              wtap_rec* rec, Buffer* buf,
                              int* err, gchar** err_info);
static gboolean vcd_read_packet(wtap* wth, FILE_T fh,
                                wtap_rec* rec, Buffer* buf,
                                int* err, gchar** err_info);

static int vcd_file_type_subtype;

/*
 * Try to interpret a file as a vcd formatted file.
 * Read relevant parts of the given file and collect information needed to
 * read the individual frames. Return value indicates whether or not this is
 * recognized as an vcd file.
 */
static wtap_open_return_val
vcd_open(wtap* wth, int* err, char** err_info)
{
    wth->subtype_read = vcd_read;
    wth->subtype_seek_read = vcd_seek_read;
    wth->file_type_subtype = vcd_file_type_subtype;
    wth->file_encap = WTAP_ENCAP_USB_FREEBSD;
    wth->file_tsprec = WTAP_TSPREC_USEC;

    return WTAP_OPEN_MINE;
}

/*
 * Sequential read with offset reporting.
 * Read the next frame in the file and adjust for the multiframe size
 * indication. Report back where reading of this frame started to
 * support subsequent random access read.
 */
static gboolean
vcd_read(wtap* wth, wtap_rec* rec, Buffer* buf, int* err, gchar** err_info,
         gint64* data_offset)
{
    /* Report the current file location */
    *data_offset = file_tell(wth->fh);

    /* Try to read a packet worth of data */
    if (!vcd_read_packet(wth, wth->fh, rec, buf, err, err_info))
        return FALSE;

    return TRUE;
}

/*
 * Random access read.
 * Read the frame at the given offset in the file. Store the frame data
 * in a buffer and fill in the packet header info.
 */
static gboolean
vcd_seek_read(wtap* wth, gint64 seek_off, wtap_rec* rec,
              Buffer* buf, int* err, gchar** err_info)
{
    /* Seek to the desired file position at the start of the frame */
    if (file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1)
        return FALSE;

    /* Try to read a packet worth of data */
    if (!vcd_read_packet(wth, wth->random_fh, rec, buf, err, err_info))
    {
        if (*err == 0)
            *err = WTAP_ERR_SHORT_READ;
        return FALSE;
    }

    return TRUE;
}

/*
 * Read the actual frame data from the file.
 * This requires reading the header, determine the size, read frame size worth
 * of data into the buffer and setting the packet header fields to the values
 * of the frame header.
 *
 * Also, for the sequential read, keep track of the position in the multiframe
 * so that we can find the next multiframe size field.
 */
static gboolean
vcd_read_packet(wtap* wth, FILE_T fh, wtap_rec* rec, Buffer* buf,
                int* err, gchar** err_info)
{
    guint8 bpf_hdr[18];
    guint8 bpf_hdr_len, alignment;

    /* Read the packet header */
    if (!wtap_read_bytes_or_eof(fh, bpf_hdr, 18, err, err_info))
        return FALSE;

    /* Get sizes */
    bpf_hdr_len = bpf_hdr[16];
    alignment = bpf_hdr[17];

    /* Check header length */
    if (bpf_hdr_len > 18)
    {
        /* Read packet header padding */
        if (!wtap_read_bytes_or_eof(fh, NULL, bpf_hdr_len - 18, err, err_info))
            return FALSE;
    }

    /* Setup the per packet structure and fill it with info from this frame */
    rec->rec_type = REC_TYPE_PACKET;
    rec->presence_flags = WTAP_HAS_TS | WTAP_HAS_CAP_LEN;
    rec->ts.secs = (guint32)bpf_hdr[3] << 24 | (guint32)bpf_hdr[2] << 16 |
        (guint32)bpf_hdr[1] << 8 | (guint32)bpf_hdr[0];
    rec->ts.nsecs = ((guint32)bpf_hdr[7] << 24 | (guint32)bpf_hdr[6] << 16 |
        (guint32)bpf_hdr[5] << 8 | (guint32)bpf_hdr[4]) * 1000;
    rec->rec_header.packet_header.caplen = (guint32)bpf_hdr[11] << 24 | (guint32)bpf_hdr[10] << 16 |
        (guint32)bpf_hdr[9] << 8 | (guint32)bpf_hdr[8];
    rec->rec_header.packet_header.len = (guint32)bpf_hdr[15] << 24 | (guint32)bpf_hdr[14] << 16 |
        (guint32)bpf_hdr[13] << 8 | (guint32)bpf_hdr[12];

    /* Read the packet data */
    if (!wtap_read_packet_bytes(fh, buf, rec->rec_header.packet_header.caplen, err, err_info))
        return FALSE;

    /* Check for and apply alignment as defined in the frame header */
    guint8 pad_len = (guint32)alignment -
    (((guint32)bpf_hdr_len + rec->rec_header.packet_header.caplen) &
        ((guint32)alignment - 1));
    if (pad_len < alignment)
    {
        /* Read alignment from the file */
        if (!wtap_read_bytes(fh, NULL, pad_len, err, err_info))
            return FALSE;
    }

    return TRUE;
}

/*
 * Register with wiretap.
 * Register how we can handle an unknown file to see if this is a valid
 * vcd file and register information about this file format.
 */
void
wtap_register_vcd(void)
{
    struct open_info oi = {
        "Value Change Dump",
        OPEN_INFO_HEURISTIC,
        vcd_open,
        NULL,
        NULL,
        NULL
    };

    wtap_register_open_info(&oi, FALSE);

    struct file_type_subtype_info fi = {
        "Value Change Dump",
        "VCD",
        "vcd",
        NULL,
        FALSE,
        FALSE,
        0,
        NULL,
        NULL,
        NULL
    };

#if VERSION_MAJOR <= 3 && VERSION_MINOR < 6
    vcd_file_type_subtype =
        wtap_register_file_type_subtypes(&fi, WTAP_FILE_TYPE_SUBTYPE_UNKNOWN);
#else
    vcd_file_type_subtype =
        wtap_register_file_type_subtype(&fi);
#endif
}

WS_DLL_PUBLIC
void plugin_register(void)
{
    static wtap_plugin plug_vcd;

    plug_vcd.register_wtap_module = wtap_register_vcd;
    wtap_register_plugin(&plug_vcd);
}
