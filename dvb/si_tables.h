//////////////////////////////////////////////////////////////
///                                                        ///
/// si_tables.h: definitions for data structures of the    ///
///              incoming SI data stream                   ///
///                                                        ///
//////////////////////////////////////////////////////////////

#include <stdint.h>

// $Revision$
// $Date$
// $Author$
//
//   (C) 2001-03 Rolf Hakenes <hakenes@hippomi.de>, under the
//               GNU GPL with contribution of Oleg Assovski,
//               www.satmania.com
//
// libsi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// libsi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You may have received a copy of the GNU General Public License
// along with libsi; see the file COPYING.  If not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

#define HILO(x) (x##_hi << 8 | x##_lo)
#define HILO2(x) (x##1 << 8 | x##2)
#define HILO3(x) (x##1 << 16 | x##2 << 8 | x##3)
#define HILO4(x) (x##4 << 24 | x##2 << 16 | x##3 << 8 | x##4)

#define MjdToEpochTime(x) ((HILO(x)-40587)*86400)
#define BcdTimeToSeconds(x) ((3600 * ((10*((x##_h & 0xF0)>>4)) + (x##_h & 0xF))) + \
                             (60 * ((10*((x##_m & 0xF0)>>4)) + (x##_m & 0xF))) + \
                             ((10*((x##_s & 0xF0)>>4)) + (x##_s & 0xF)))
#define BcdTimeToMinutes(x) ((60 * ((10*((x##_h & 0xF0)>>4)) + (x##_h & 0xF))) + \
                             (((10*((x##_m & 0xF0)>>4)) + (x##_m & 0xF))))
#define BcdCharToInt(x) (10*((x & 0xF0)>>4) + (x & 0xF))
#define CheckBcdChar(x) ((((x & 0xF0)>>4) <= 9) && \
                         ((x & 0x0F) <= 9))
#define CheckBcdSignedChar(x) ((((x & 0xF0)>>4) >= 0) && (((x & 0xF0)>>4) <= 9) && \
                         ((x & 0x0F) >= 0) && ((x & 0x0F) <= 9))

#define GetTableId(x) (((si_tab_t *)(x))->table_id)
#define GetSectionLength(x) HILO(((si_tab_t *)(x))->section_length)
typedef struct si_tab {
   unsigned char table_id                               /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char section_syntax_indicator               :1;
   unsigned char dummy                                  :1;        // has to be 0
   unsigned char                                        :2;
   unsigned char section_length_hi                      :4;
#else
   unsigned char section_length_hi                      :4;
   unsigned char                                        :2;
   unsigned char dummy                                  :1;        // has to be 0
   unsigned char section_syntax_indicator               :1;
#endif
   unsigned char section_length_lo                      /*:8*/;
} si_tab_t;

/*
 *
 *    ETSI ISO/IEC 13818-1 specifies SI which is referred to as PSI. The PSI
 *    data provides information to enable automatic configuration of the
 *    receiver to demultiplex and decode the various streams of programs
 *    within the multiplex. The PSI data is structured as four types of table.
 *    The tables are transmitted in sections.
 *
 *    1) Program Association Table (PAT):
 *
 *       - for each service in the multiplex, the PAT indicates the location
 *         (the Packet Identifier (PID) values of the Transport Stream (TS)
 *         packets) of the corresponding Program Map Table (PMT).
 *         It also gives the location of the Network Information Table (NIT).
 *
 */
#define TableHasMoreSections(x) (((pat_t *)(x))->last_section_number > ((pat_t *)(x))->section_number)
#define GetSectionNumber(x) ((pat_t *)(x))->section_number
#define GetLastSectionNumber(x) ((pat_t *)(x))->last_section_number
#define GetServiceId(x) HILO(((eit_t *)(x))->service_id)
#define GetLastTableId(x) ((eit_t *)(x))->segment_last_table_id
#define GetSegmentLastSectionNumber(x) ((eit_t *)(x))->segment_last_section_number
typedef struct pat {
   unsigned char table_id                               /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char section_syntax_indicator               :1;
   unsigned char dummy                                  :1;        // has to be 0
   unsigned char                                        :2;
   unsigned char section_length_hi                      :4;
#else
   unsigned char section_length_hi                      :4;
   unsigned char                                        :2;
   unsigned char dummy                                  :1;        // has to be 0
   unsigned char section_syntax_indicator               :1;
#endif
   unsigned char section_length_lo                      /*:8*/;
   unsigned char transport_stream_id_hi                 /*:8*/;
   unsigned char transport_stream_id_lo                 /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char version_number                         :5;
   unsigned char current_next_indicator                 :1;
#else
   unsigned char current_next_indicator                 :1;
   unsigned char version_number                         :5;
   unsigned char                                        :2;
#endif
   unsigned char section_number                         /*:8*/;
   unsigned char last_section_number                    /*:8*/;
} pat_t;
#define PAT_LEN sizeof (pat_t)

typedef struct pat_prog {
   unsigned char program_number_hi                      /*:8*/;
   unsigned char program_number_lo                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :3;
   unsigned char network_pid_hi                         :5;
#else
   unsigned char network_pid_hi                         :5;
   unsigned char                                        :3;
#endif
   unsigned char network_pid_lo                         /*:8*/;
   /* or program_map_pid (if prog_num=0)*/
} pat_prog_t;
#define PAT_PROG_LEN sizeof (pat_prog_t)

/*
 *
 *    2) Conditional Access Table (CAT):
 *
 *       - the CAT provides information on the CA systems used in the
 *         multiplex; the information is private and dependent on the CA
 *         system, but includes the location of the EMM stream, when
 *         applicable.
 *
 */
typedef struct cat {
   unsigned char table_id                               /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char section_syntax_indicator               :1;
   unsigned char dummy                                  :1;        // has to be 0
   unsigned char                                        :2;
   unsigned char section_length_hi                      :4;
#else
   unsigned char section_length_hi                      :4;
   unsigned char                                        :2;
   unsigned char dummy                                  :1;        // has to be 0
   unsigned char section_syntax_indicator               :1;
#endif
   unsigned char section_length_lo                      /*:8*/;
   unsigned char                                        :8;
   unsigned char                                        :8;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char version_number                         :5;
   unsigned char current_next_indicator                 :1;
#else
   unsigned char current_next_indicator                 :1;
   unsigned char version_number                         :5;
   unsigned char                                        :2;
#endif
   unsigned char section_number                         /*:8*/;
   unsigned char last_section_number                    /*:8*/;
} cat_t;
#define CAT_LEN sizeof (cat_t)

/*
 *
 *    3) Program Map Table (PMT):
 *
 *       - the PMT identifies and indicates the locations of the streams that
 *         make up each service, and the location of the Program Clock
 *         Reference fields for a service.
 *
 */
typedef struct pmt {
   unsigned char table_id                               /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char section_syntax_indicator               :1;
   unsigned char dummy                                  :1; // has to be 0
   unsigned char                                        :2;
   unsigned char section_length_hi                      :4;
#else
   unsigned char section_length_hi                      :4;
   unsigned char                                        :2;
   unsigned char dummy                                  :1; // has to be 0
   unsigned char section_syntax_indicator               :1;
#endif
   unsigned char section_length_lo                      /*:8*/;
   unsigned char program_number_hi                      /*:8*/;
   unsigned char program_number_lo                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char version_number                         :5;
   unsigned char current_next_indicator                 :1;
#else
   unsigned char current_next_indicator                 :1;
   unsigned char version_number                         :5;
   unsigned char                                        :2;
#endif
   unsigned char section_number                         /*:8*/;
   unsigned char last_section_number                    /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :3;
   unsigned char PCR_PID_hi                             :5;
#else
   unsigned char PCR_PID_hi                             :5;
   unsigned char                                        :3;
#endif
   unsigned char PCR_PID_lo                             /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :4;
   unsigned char program_info_length_hi                 :4;
#else
   unsigned char program_info_length_hi                 :4;
   unsigned char                                        :4;
#endif
   unsigned char program_info_length_lo                 /*:8*/;
   //descriptors
} pmt_t;
#define PMT_LEN sizeof (pmt_t)

typedef struct pmt_info {
   unsigned char stream_type                            /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :3;
   unsigned char elementary_PID_hi                      :5;
#else
   unsigned char elementary_PID_hi                      :5;
   unsigned char                                        :3;
#endif
   unsigned char elementary_PID_lo                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :4;
   unsigned char ES_info_length_hi                      :4;
#else
   unsigned char ES_info_length_hi                      :4;
   unsigned char                                        :4;
#endif
   unsigned char ES_info_length_lo                      /*:8*/;
   // descriptors
} pmt_info_t;
#define PMT_INFO_LEN sizeof (pmt_info_t)

/*
 *
 *    4) Network Information Table (NIT):
 *
 *       - the NIT is intended to provide information about the physical
 *         network. The syntax and semantics of the NIT are defined in
 *         ETSI EN 300 468.
 *
 */
typedef struct nit {
   unsigned char table_id                               /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char section_syntax_indicator               :1;
   unsigned char                                        :3;
   unsigned char section_length_hi                      :4;
#else
   unsigned char section_length_hi                      :4;
   unsigned char                                        :3;
   unsigned char section_syntax_indicator               :1;
#endif
   unsigned char section_length_lo                      /*:8*/;
   unsigned char network_id_hi                          /*:8*/;
   unsigned char network_id_lo                          /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char version_number                         :5;
   unsigned char current_next_indicator                 :1;
#else
   unsigned char current_next_indicator                 :1;
   unsigned char version_number                         :5;
   unsigned char                                        :2;
#endif
   unsigned char section_number                         /*:8*/;
   unsigned char last_section_number                    /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :4;
   unsigned char network_descriptor_length_hi           :4;
#else
   unsigned char network_descriptor_length_hi           :4;
   unsigned char                                        :4;
#endif
   unsigned char network_descriptor_length_lo           /*:8*/;
   unsigned char data[];
  /* descriptors */
} nit_t;
#define NIT_LEN sizeof (nit_t)
#define GetNITDescriptorsLoopLength(x) HILO(((nit_t *)x)->network_descriptor_length)

typedef struct nit_mid {                                 // after descriptors
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :4;
   unsigned char transport_stream_loop_length_hi        :4;
#else
   unsigned char transport_stream_loop_length_hi        :4;
   unsigned char                                        :4;
#endif
   unsigned char transport_stream_loop_length_lo        /*:8*/;
} nit_mid_t;
#define SIZE_NIT_MID sizeof (nit_mid_t)

typedef struct nit_end {
   long CRC;
} nit_end_t;
#define SIZE_NIT_END sizeof (nit_end_t)

typedef struct nit_ts {
   unsigned char transport_stream_id_hi                 /*:8*/;
   unsigned char transport_stream_id_lo                 /*:8*/;
   unsigned char original_network_id_hi                 /*:8*/;
   unsigned char original_network_id_lo                 /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :4;
   unsigned char transport_descriptors_length_hi        :4;
#else
   unsigned char transport_descriptors_length_hi        :4;
   unsigned char                                        :4;
#endif
   unsigned char transport_descriptors_length_lo        /*:8*/;
   /* descriptors  */
} nit_ts_t;
#define NIT_TS_LEN sizeof (nit_ts_t)

/*
 *
 *    In addition to the PSI, data is needed to provide identification of
 *    services and events for the user. In contrast with the PAT, CAT, and
 *    PMT of the PSI, which give information only for the multiplex in which
 *    they are contained (the actual multiplex), the additional information
 *    defined within the present document can also provide information on
 *    services and events carried by different multiplexes, and even on other
 *    networks. This data is structured as nine tables:
 *
 *    1) Bouquet Association Table (BAT):
 *
 *       - the BAT provides information regarding bouquets. As well as giving
 *         the name of the bouquet, it provides a list of services for each
 *         bouquet.
 *
 */
/* SEE NIT (It has the same structure but has different allowed descriptors) */
/*
 *
 *    2) Service Description Table (SDT):
 *
 *       - the SDT contains data describing the services in the system e.g.
 *         names of services, the service provider, etc.
 *
 */
typedef struct sdt {
   unsigned char table_id                               /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char section_syntax_indicator               :1;
   unsigned char                                        :3;
   unsigned char section_length_hi                      :4;
#else
   unsigned char section_length_hi                      :4;
   unsigned char                                        :3;
   unsigned char section_syntax_indicator               :1;
#endif
   unsigned char section_length_lo                      /*:8*/;
   unsigned char transport_stream_id_hi                 /*:8*/;
   unsigned char transport_stream_id_lo                 /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char version_number                         :5;
   unsigned char current_next_indicator                 :1;
#else
   unsigned char current_next_indicator                 :1;
   unsigned char version_number                         :5;
   unsigned char                                        :2;
#endif
   unsigned char section_number                         /*:8*/;
   unsigned char last_section_number                    /*:8*/;
   unsigned char original_network_id_hi                 /*:8*/;
   unsigned char original_network_id_lo                 /*:8*/;
   unsigned char                                        :8;
} sdt_t;
#define SDT_LEN sizeof (sdt_t)
#define GetSDTTransportStreamId(x) HILO(((sdt_t *)x)->transport_stream_id)
#define GetSDTOriginalNetworkId(x) HILO(((sdt_t *)x)->original_network_id)

typedef struct sdt_descr {
   unsigned char service_id_hi                          /*:8*/;
   unsigned char service_id_lo                          /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :6;
   unsigned char eit_schedule_flag                      :1;
   unsigned char eit_present_following_flag             :1;
   unsigned char running_status                         :3;
   unsigned char free_ca_mode                           :1;
   unsigned char descriptors_loop_length_hi             :4;
#else
   unsigned char eit_present_following_flag             :1;
   unsigned char eit_schedule_flag                      :1;
   unsigned char                                        :6;
   unsigned char descriptors_loop_length_hi             :4;
   unsigned char free_ca_mode                           :1;
   unsigned char running_status                         :3;
#endif
   unsigned char descriptors_loop_length_lo             /*:8*/;
} sdt_descr_t;
#define SDT_DESCR_LEN sizeof (sdt_descr_t)
#define GetSDTDescriptorsLoopLength(x) HILO(((sdt_descr_t *)x)->descriptors_loop_length)

/*
 *
 *    3) Event Information Table (EIT):
 *
 *       - the EIT contains data concerning events or programmes such as event
 *         name, start time, duration, etc.; - the use of different descriptors
 *         allows the transmission of different kinds of event information e.g.
 *         for different service types.
 *
 */
typedef struct eit {
   unsigned char table_id                               /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char section_syntax_indicator               :1;
   unsigned char                                        :3;
   unsigned char section_length_hi                      :4;
#else
   unsigned char section_length_hi                      :4;
   unsigned char                                        :3;
   unsigned char section_syntax_indicator               :1;
#endif
   unsigned char section_length_lo                      /*:8*/;
   unsigned char service_id_hi                          /*:8*/;
   unsigned char service_id_lo                          /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char version_number                         :5;
   unsigned char current_next_indicator                 :1;
#else
   unsigned char current_next_indicator                 :1;
   unsigned char version_number                         :5;
   unsigned char                                        :2;
#endif
   unsigned char section_number                         /*:8*/;
   unsigned char last_section_number                    /*:8*/;
   unsigned char transport_stream_id_hi                 /*:8*/;
   unsigned char transport_stream_id_lo                 /*:8*/;
   unsigned char original_network_id_hi                 /*:8*/;
   unsigned char original_network_id_lo                 /*:8*/;
   unsigned char segment_last_section_number            /*:8*/;
   unsigned char segment_last_table_id                  /*:8*/;
   unsigned char data[]; /* struct eit_event */
} eit_t;
#define EIT_LEN sizeof (eit_t)

typedef struct eit_event {
   unsigned char event_id_hi                            /*:8*/;
   unsigned char event_id_lo                            /*:8*/;
   unsigned char mjd_hi                                 /*:8*/;
   unsigned char mjd_lo                                 /*:8*/;
   unsigned char start_time_h                           /*:8*/;
   unsigned char start_time_m                           /*:8*/;
   unsigned char start_time_s                           /*:8*/;
   unsigned char duration_h                             /*:8*/;
   unsigned char duration_m                             /*:8*/;
   unsigned char duration_s                             /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char running_status                         :3;
   unsigned char free_ca_mode                           :1;
   unsigned char descriptors_loop_length_hi             :4;
#else
   unsigned char descriptors_loop_length_hi             :4;
   unsigned char free_ca_mode                           :1;
   unsigned char running_status                         :3;
#endif
   unsigned char descriptors_loop_length_lo             /*:8*/;
   unsigned char data[]; /* struct descr_gen */
} eit_event_t;
#define EIT_EVENT_LEN sizeof (eit_event_t)
#define GetEITDescriptorsLoopLength(x) HILO(((eit_event_t *)x)->descriptors_loop_length)

/*
 *
 *    4) Running Status Table (RST):
 *
 *       - the RST gives the status of an event (running/not running). The RST
 *         updates this information and allows timely automatic switching to
 *         events.
 *
 */
    /* TO BE DONE */
/*
 *
 *    5) Time and Date Table (TDT):
 *
 *       - the TDT gives information relating to the present time and date.
 *         This information is given in a separate table due to the frequent
 *         updating of this information.
 *
 */

typedef struct tdt {
   unsigned char table_id                               /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char section_syntax_indicator               :1;
   unsigned char                                        :3;
   unsigned char section_length_hi                      :4;
#else
   unsigned char section_length_hi                      :4;
   unsigned char                                        :3;
   unsigned char section_syntax_indicator               :1;
#endif
   unsigned char section_length_lo                      /*:8*/;
   unsigned char utc_mjd_hi                             /*:8*/;
   unsigned char utc_mjd_lo                             /*:8*/;
   unsigned char utc_time_h                             /*:8*/;
   unsigned char utc_time_m                             /*:8*/;
   unsigned char utc_time_s                             /*:8*/;
} tdt_t;
#define TDT_LEN sizeof (tdt_t)

/*
 *
 *    6) Time Offset Table (TOT):
 *
 *       - the TOT gives information relating to the present time and date and
 *         local time offset. This information is given in a separate table due
 *         to the frequent updating of the time information.
 *
 */
typedef struct tot {
   unsigned char table_id                               /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char section_syntax_indicator               :1;
   unsigned char                                        :3;
   unsigned char section_length_hi                      :4;
#else
   unsigned char section_length_hi                      :4;
   unsigned char                                        :3;
   unsigned char section_syntax_indicator               :1;
#endif
   unsigned char section_length_lo                      /*:8*/;
   unsigned char utc_mjd_hi                             /*:8*/;
   unsigned char utc_mjd_lo                             /*:8*/;
   unsigned char utc_time_h                             /*:8*/;
   unsigned char utc_time_m                             /*:8*/;
   unsigned char utc_time_s                             /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :4;
   unsigned char descriptors_loop_length_hi             :4;
#else
   unsigned char descriptors_loop_length_hi             :4;
   unsigned char                                        :4;
#endif
   unsigned char descriptors_loop_length_lo             /*:8*/;
} tot_t;
#define TOT_LEN sizeof (tot_t)

/*
 *
 *    7) Stuffing Table (ST):
 *
 *       - the ST is used to invalidate existing sections, for example at
 *         delivery system boundaries.
 *
 */
    /* TO BE DONE */
/*
 *
 *    8) Selection Information Table (SIT):
 *
 *       - the SIT is used only in "partial" (i.e. recorded) bitstreams. It
 *         carries a summary of the SI information required to describe the
 *         streams in the partial bitstream.
 *
 */
    /* TO BE DONE */
/*
 *
 *    9) Discontinuity Information Table (DIT):
 *
 *       - the DIT is used only in "partial" (i.e. recorded) bitstreams.
 *         It is inserted where the SI information in the partial bitstream may
 *         be discontinuous. Where applicable the use of descriptors allows a
 *         flexible approach to the organization of the tables and allows for
 *         future compatible extensions.
 *
 */
    /* TO BE DONE */
/*
 *
 *    The following describes the different descriptors that can be used within
 *    the SI.
 *
 *    The following semantics apply to all the descriptors defined in this
 *    subclause:
 *
 *    descriptor_tag: The descriptor tag is an 8-bit field which identifies
 *                    each descriptor. Those values with MPEG-2 normative
 *                    meaning are described in ISO/IEC 13818-1. The values of
 *                    descriptor_tag are defined in 'libsi.h'
 *    descriptor_length: The descriptor length is an 8-bit field specifying the
 *                       total number of bytes of the data portion of the
 *                       descriptor following the byte defining the value of
 *                       this field.
 *
 */

typedef struct descr_gen {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
} descr_gen_t;
#define DESCR_GEN_LEN sizeof (descr_gen_t)
#define CastGenericDescriptor(x) ((descr_gen_t *)(x))

#define GetDescriptorTag(x) (((descr_gen_t *)x)->descriptor_tag)
#define GetDescriptorLength(x) (((descr_gen_t *)x)->descriptor_length)

/* 0x09 ca_descriptor */
typedef struct descr_ca {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char CA_type_hi                             /*:8*/;
   unsigned char CA_type_lo                             /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :3;
   unsigned char CA_PID_hi                              :5;
#else
   unsigned char CA_PID_hi                              :5;
   unsigned char                                        :3;
#endif
   unsigned char CA_PID_lo                              /*:8*/;
} descr_ca_t;
#define DESCR_CA_LEN sizeof (descr_ca_t)
#define CastCaDescriptor(x) ((descr_ca_t *)(x))

/* 0x0A iso_639_language_descriptor */
typedef struct descr_iso_639_language {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char lang_code1                             /*:8*/;
   unsigned char lang_code2                             /*:8*/;
   unsigned char lang_code3                             /*:8*/;
} descr_iso_639_language_t;
#define DESCR_ISO_639_LANGUAGE_LEN sizeof (descr_iso_639_language_t)
#define CastIso639LanguageDescriptor(x) ((descr_iso_639_language_t *)(x))

/* 0x40 network_name_descriptor */
typedef struct descr_network_name {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
} descr_network_name_t;
#define DESCR_NETWORK_NAME_LEN sizeof (descr_network_name_t)
#define CastNetworkNameDescriptor(x) ((descr_network_name_t *)(x))

/* 0x41 service_list_descriptor */
typedef struct descr_service_list {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
} descr_service_list_t;
#define DESCR_SERVICE_LIST_LEN sizeof (descr_service_list_t)
#define CastServiceListDescriptor(x) ((descr_service_list_t *)(x))

typedef struct descr_service_list_loop {
   unsigned char service_id_hi                          /*:8*/;
   unsigned char service_id_lo                          /*:8*/;
   unsigned char service_type                           /*:8*/;
} descr_service_list_loop_t;
#define DESCR_SERVICE_LIST_LOOP_LEN sizeof (descr_service_list_loop_t)
#define CastServiceListDescriptorLoop(x) ((descr_service_list_loop_t *)(x))

/* 0x42 stuffing_descriptor */
typedef struct descr_stuffing {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[];
} descr_stuffing_t;
#define DESCR_STUFFING_LEN sizeof (descr_stuffing_t)
#define CastStuffingDescriptor(x) ((descr_stuffing_t *)(x))

/* 0x43 satellite_delivery_system_descriptor */
typedef struct descr_satellite_delivery_system {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char frequency1                             /*:8*/;
   unsigned char frequency2                             /*:8*/;
   unsigned char frequency3                             /*:8*/;
   unsigned char frequency4                             /*:8*/;
   unsigned char orbital_position1                      /*:8*/;
   unsigned char orbital_position2                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char west_east_flag                         :1;
   unsigned char polarization                           :2;
   unsigned char modulation                             :5;
#else
   unsigned char modulation                             :5;
   unsigned char polarization                           :2;
   unsigned char west_east_flag                         :1;
#endif
   unsigned char symbol_rate1                           /*:8*/;
   unsigned char symbol_rate2                           /*:8*/;
   unsigned char symbol_rate3                           /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char symbol_rate4                           :4;
   unsigned char fec_inner                              :4;
#else
   unsigned char fec_inner                              :4;
   unsigned char symbol_rate4                           :4;
#endif
} descr_satellite_delivery_system_t;
#define DESCR_SATELLITE_DELIVERY_SYSTEM_LEN sizeof (descr_satellite_delivery_system_t)
#define CastSatelliteDeliverySystemDescriptor(x) ((descr_satellite_delivery_system_t *)(x))
#define GetSatelliteDeliverySystemFrequency(x) HILO4(((descr_satellite_delivert_system_t *)x)->frequency)
#define GetSatelliteDeliverySystemSymbolRate(x) HILO4(((descr_satellite_delivert_system_t *)x)->symbol_rate)


/* 0x44 cable_delivery_system_descriptor */
typedef struct descr_cable_delivery_system {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char frequency1                             /*:8*/;
   unsigned char frequency2                             /*:8*/;
   unsigned char frequency3                             /*:8*/;
   unsigned char frequency4                             /*:8*/;
   unsigned char                                        :8;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :4;
   unsigned char fec_outer                              :4;
#else
   unsigned char fec_outer                              :4;
   unsigned char                                        :4;
#endif
   unsigned char modulation                             /*:8*/;
   unsigned char symbol_rate1                           /*:8*/;
   unsigned char symbol_rate2                           /*:8*/;
   unsigned char symbol_rate3                           /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char symbol_rate4                           :4;
   unsigned char fec_inner                              :4;
#else
   unsigned char fec_inner                              :4;
   unsigned char symbol_rate4                           :4;
#endif
} descr_cable_delivery_system_t;
#define DESCR_CABLE_DELIVERY_SYSTEM_LEN sizeof (descr_cable_delivery_system_t)
#define CastCableDeliverySystemDescriptor(x) ((descr_cable_delivery_system_t *)(x))
#define GetCableDeliverySystemFrequency(x) HILO4(((descr_cable_delivert_system_t *)x)->frequency)
#define GetCableDeliverySystemSymbolRate(x) HILO4(((descr_cable_delivert_system_t *)x)->symbol_rate)

/* 0x45 vbi_data_descriptor */
typedef struct descr_vbi_data {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   /* TBD */
} descr_vbi_data_t;
#define DESCR_VBI_DATA_LEN sizeof (descr_vbi_data_t)
#define CastVbiDataDescriptor(x) ((descr_vbi_data_t *)(x))

/* 0x46 vbi_teletext_descriptor */
typedef struct descr_vbi_teletext {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   /* TBD */
} descr_vbi_teletext_t;
#define DESCR_VBI_TELETEXT_LEN sizeof (descr_vbi_teletext_t)
#define CastVbiDescriptor(x) ((descr_vbi_teletext_t *)(x))

/* 0x47 bouquet_name_descriptor */
typedef struct descr_bouquet_name {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
} descr_bouquet_name_t;
#define DESCR_BOUQUET_NAME_LEN sizeof (descr_bouquet_name_t)

#define CastBouquetNameDescriptor(x) ((descr_bouquet_name_t *)(x))

/* 0x48 service_descriptor */
typedef struct descr_service {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char service_type                           /*:8*/;
   unsigned char provider_name_length                   /*:8*/;
} descr_service_t;
#define DESCR_SERVICE_LEN  sizeof (descr_service_t)
#define CastServiceDescriptor(x) ((descr_service_t *)(x))

/* 0x49 country_availability_descriptor */
typedef struct descr_country_availability {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char country_availability_flag              :1;
   unsigned char                                        :7;
#else
   unsigned char                                        :7;
   unsigned char country_availability_flag              :1;
#endif
} descr_country_availability_t;
#define DESCR_COUNTRY_AVAILABILITY_LEN sizeof (descr_country_availability_t)
#define CastCountryAvailabilityDescriptor(x) ((descr_country_availability_t *)(x))

/* 0x4A linkage_descriptor */
typedef struct descr_linkage {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char transport_stream_id_hi                 /*:8*/;
   unsigned char transport_stream_id_lo                 /*:8*/;
   unsigned char original_network_id_hi                 /*:8*/;
   unsigned char original_network_id_lo                 /*:8*/;
   unsigned char service_id_hi                          /*:8*/;
   unsigned char service_id_lo                          /*:8*/;
   unsigned char linkage_type                           /*:8*/;
   unsigned char data[];
} descr_linkage_t;
#define DESCR_LINKAGE_LEN sizeof (descr_linkage_t)
#define CastLinkageDescriptor(x) ((descr_linkage_t *)(x))

/* 0x4B nvod_reference_descriptor */
typedef struct descr_nvod_reference {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[]; /* struct item_nvod_reference */
} descr_nvod_reference_t;
#define DESCR_NVOD_REFERENCE_LEN sizeof (descr_nvod_reference_t)
#define CastNvodReferenceDescriptor(x) ((descr_nvod_reference_t *)(x))

typedef struct item_nvod_reference {
   unsigned char transport_stream_id_hi                 /*:8*/;
   unsigned char transport_stream_id_lo                 /*:8*/;
   unsigned char original_network_id_hi                 /*:8*/;
   unsigned char original_network_id_lo                 /*:8*/;
   unsigned char service_id_hi                          /*:8*/;
   unsigned char service_id_lo                          /*:8*/;
} item_nvod_reference_t;
#define ITEM_NVOD_REFERENCE_LEN sizeof (item_nvod_reference_t)
#define CastNvodReferenceItem(x) ((item_nvod_reference_t *)(x))

/* 0x4C time_shifted_service_descriptor */
typedef struct descr_time_shifted_service {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char reference_service_id_hi                /*:8*/;
   unsigned char reference_service_id_lo                /*:8*/;
} descr_time_shifted_service_t;
#define DESCR_TIME_SHIFTED_SERVICE_LEN sizeof (descr_time_shifted_service_t)
#define CastTimeShiftedServiceDescriptor(x) ((descr_time_shifted_service_t *)(x))

/* 0x4D short_event_descriptor */
typedef struct descr_short_event {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char lang_code1                             /*:8*/;
   unsigned char lang_code2                             /*:8*/;
   unsigned char lang_code3                             /*:8*/;
   unsigned char event_name_length                      /*:8*/;
   unsigned char data[];
} descr_short_event_t;
#define DESCR_SHORT_EVENT_LEN sizeof (descr_short_event_t)
#define CastShortEventDescriptor(x) ((descr_short_event_t *)(x))

/* 0x4E extended_event_descriptor */
typedef struct descr_extended_event {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char descriptor_number                      :4;
   unsigned char last_descriptor_number                 :4;
#else
   unsigned char last_descriptor_number                 :4;
   unsigned char descriptor_number                      :4;
#endif
   unsigned char lang_code1                             /*:8*/;
   unsigned char lang_code2                             /*:8*/;
   unsigned char lang_code3                             /*:8*/;
   unsigned char length_of_items                        /*:8*/;
   unsigned char data[]; /* struct item_extended_event */
} descr_extended_event_t;
#define DESCR_EXTENDED_EVENT_LEN sizeof (descr_extended_event_t)
#define CastExtendedEventDescriptor(x) ((descr_extended_event_t *)(x))

typedef struct item_extended_event {
   unsigned char item_description_length               /*:8*/;
   unsigned char data[];
} item_extended_event_t;
#define ITEM_EXTENDED_EVENT_LEN sizeof (item_extended_event_t)
#define CastExtendedEventItem(x) ((item_extended_event_t *)(x))

/* 0x4F time_shifted_event_descriptor */
typedef struct descr_time_shifted_event {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char reference_service_id_hi                /*:8*/;
   unsigned char reference_service_id_lo                /*:8*/;
   unsigned char reference_event_id_hi                  /*:8*/;
   unsigned char reference_event_id_lo                  /*:8*/;
} descr_time_shifted_event_t;
#define DESCR_TIME_SHIFTED_EVENT_LEN sizeof (descr_time_shifted_event_t)
#define CastTimeShiftedEventDescriptor(x) ((descr_time_shifted_event_t *)(x))

/* 0x50 component_descriptor */
typedef struct descr_component {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :4;
   unsigned char stream_content                         :4;
#else
   unsigned char stream_content                         :4;
   unsigned char                                        :4;
#endif
   unsigned char component_type                         /*:8*/;
   unsigned char component_tag                          /*:8*/;
   unsigned char lang_code1                             /*:8*/;
   unsigned char lang_code2                             /*:8*/;
   unsigned char lang_code3                             /*:8*/;
   unsigned char data[];
} descr_component_t;
#define DESCR_COMPONENT_LEN  sizeof (descr_component_t)
#define CastComponentDescriptor(x) ((descr_component_t *)(x))

/* 0x51 mosaic_descriptor */
typedef struct descr_mosaic {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char mosaic_entry_point                     :1;
   unsigned char number_of_horizontal_elementary_cells  :3;
   unsigned char                                        :1;
   unsigned char number_of_vertical_elementary_cells    :3;
#else
   unsigned char number_of_vertical_elementary_cells    :3;
   unsigned char                                        :1;
   unsigned char number_of_horizontal_elementary_cells  :3;
   unsigned char mosaic_entry_point                     :1;
#endif
   unsigned char data[]; /* struct item_mosaic */
} descr_mosaic_t;
#define DESCR_MOSAIC_LEN sizeof (descr_mosaic_t)
#define CastMosaicDescriptor(x) ((descr_mosaic_t *)(x))

typedef struct item_mosaic {
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char logical_cell_id                        :6;
   unsigned char                                        :7;
   unsigned char logical_cell_presentation_info         :3;
#else
   unsigned char                                        :2;
   unsigned char logical_cell_id                        :6;
   unsigned char logical_cell_presentation_info         :3; /*0=undefined, 1=video, 2=still picture, 3=graphical text, 4--7=reserved*/
   unsigned char                                        :5;
#endif
   unsigned char elementary_cell_field_length           /*:8*/;
   unsigned char data[]; /* struct item_mosaic_cell; struct item_mosaic_end */
} item_mosaic_t;
typedef struct item_mosaic_end {
   unsigned char cell_linkage_info                      /*:8*/; /*0=undefined, 1=bouquet, 2=service, 3=other mosaic, 4=event, 5--255=reserved*/
   unsigned char data[]; /* union item_cell_linkage */
} item_mosaic_end_t;

typedef struct item_mosaic_cell {
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char elementary_cell_id                     :6;
#else
   unsigned char elementary_cell_id                     :6;
   unsigned char                                        :2;
#endif
} item_mosaic_cell_t;
typedef union item_mosaic_cell_linkage {
   struct item_mosaic_cell_bouquet {
      unsigned char bouquet_id_hi                       /*:8*/;
      unsigned char bouquet_id_lo                       /*:8*/;
   } bouquet;
   struct item_mosaic_cell_service {
      unsigned char original_network_id_hi              /*:8*/;
      unsigned char original_network_id_lo              /*:8*/;
      unsigned char transport_stream_id_hi              /*:8*/;
      unsigned char transport_stream_id_lo              /*:8*/;
      unsigned char service_id_hi                       /*:8*/;
      unsigned char service_id_lo                       /*:8*/;
   } service;
   struct item_mosaic_cell_other {
      unsigned char original_network_id_hi              /*:8*/;
      unsigned char original_network_id_lo              /*:8*/;
      unsigned char transport_stream_id_hi              /*:8*/;
      unsigned char transport_stream_id_lo              /*:8*/;
      unsigned char service_id_hi                       /*:8*/;
      unsigned char service_id_lo                       /*:8*/;
   } other;
   struct item_mosaic_cell_event {
      unsigned char original_network_id_hi              /*:8*/;
      unsigned char original_network_id_lo              /*:8*/;
      unsigned char transport_stream_id_hi              /*:8*/;
      unsigned char transport_stream_id_lo              /*:8*/;
      unsigned char service_id_hi                       /*:8*/;
      unsigned char service_id_lo                       /*:8*/;
      unsigned char event_id_hi                         /*:8*/;
      unsigned char event_id_lo                         /*:8*/;
   } event;
} item_mosaic_cell_linkage_t;

/* 0x52 stream_identifier_descriptor */
typedef struct descr_stream_identifier {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char component_tag                          /*:8*/;
} descr_stream_identifier_t;
#define DESCR_STREAM_IDENTIFIER_LEN sizeof (descr_stream_identifier_t)
#define CastStreamIdentifierDescriptor(x) ((descr_stream_identifier_t *)(x))

/* 0x53 ca_identifier_descriptor */
typedef struct descr_ca_identifier {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
} descr_ca_identifier_t;
#define DESCR_CA_IDENTIFIER_LEN sizeof (descr_ca_identifier_t)
#define CastCaIdentifierDescriptor(x) ((descr_ca_identifier_t *)(x))

/* 0x54 content_descriptor */
typedef struct descr_content {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[]; /* struct nibble_content */
} descr_content_t;
#define DESCR_CONTENT_LEN sizeof (descr_content_t)
#define CastContentDescriptor(x) ((descr_content_t *)(x))

typedef struct nibble_content {
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char content_nibble_level_1                 :4;
   unsigned char content_nibble_level_2                 :4;
   unsigned char user_nibble_1                          :4;
   unsigned char user_nibble_2                          :4;
#else
   unsigned char user_nibble_2                          :4;
   unsigned char user_nibble_1                          :4;
   unsigned char content_nibble_level_2                 :4;
   unsigned char content_nibble_level_1                 :4;
#endif
} nibble_content_t;
#define NIBBLE_CONTENT_LEN sizeof (nibble_content_t)
#define CastContentNibble(x) ((nibble_content_t *)(x))

/* 0x55 parental_rating_descriptor */
typedef struct descr_parental_rating {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[]; /* struct parental_rating_item */
} descr_parental_rating_t;
#define DESCR_PARENTAL_RATING_LEN sizeof (descr_parental_rating_t)
#define CastParentalRatingDescriptor(x) ((descr_parental_rating_t *)(x))

typedef struct parental_rating_item {
   unsigned char lang_code1                             /*:8*/;
   unsigned char lang_code2                             /*:8*/;
   unsigned char lang_code3                             /*:8*/;
   unsigned char rating                                 /*:8*/;
} parental_rating_item_t;
#define PARENTAL_RATING_ITEM_LEN sizeof (parental_rating_item_t)
#define CastParentalRatingItem(x) ((parental_rating_item_t *)(x))

/* 0x56 teletext_descriptor */
typedef struct descr_teletext {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[]; /* struct item_teletext */
} descr_teletext_t;
#define DESCR_TELETEXT_LEN sizeof (descr_teletext_t)
#define CastTeletextDescriptor(x) ((descr_teletext_t *)(x))

typedef struct item_teletext {
   unsigned char lang_code1                             /*:8*/;
   unsigned char lang_code2                             /*:8*/;
   unsigned char lang_code3                             /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char type                                   :5;
   unsigned char magazine_number                        :3;
#else
   unsigned char magazine_number                        :3;
   unsigned char type                                   :5;
#endif
   unsigned char page_number                            /*:8*/;
} item_teletext_t;
#define ITEM_TELETEXT_LEN sizeof (item_teletext_t)
#define CastTeletextItem(x) ((item_teletext_t *)(x))

/* 0x57 telephone_descriptor */
typedef struct descr_telephone {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char foreign_availability                   :1;
   unsigned char connection_type                        :5;
#else
   unsigned char connection_type                        :5;
   unsigned char foreign_availability                   :1;
   unsigned char                                        :2;
#endif
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :1;
   unsigned char country_prefix_length                  :2;
   unsigned char international_area_code_length         :3;
   unsigned char operator_code_length                   :2;
#else
   unsigned char operator_code_length                   :2;
   unsigned char international_area_code_length         :3;
   unsigned char country_prefix_length                  :2;
   unsigned char                                        :1;
#endif
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :1;
   unsigned char national_area_code_length              :3;
   unsigned char core_number_length                     :4;
#else
   unsigned char core_number_length                     :4;
   unsigned char national_area_code_length              :3;
   unsigned char                                        :1;
#endif
   unsigned char data[]; /* coutry area operator national core */
} descr_telephone_t;
#define DESCR_TELEPHONE_LEN sizeof (descr_telephone_t)
#define CastTelephoneDescriptor(x) ((descr_telephone_t *)(x))

/* 0x58 local_time_offset_descriptor */
typedef struct descr_local_time_offset {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
} descr_local_time_offset_t;
#define DESCR_LOCAL_TIME_OFFSET_LEN sizeof (descr_local_time_offset_t)
#define CastLocalTimeOffsetDescriptor(x) ((descr_local_time_offset_t *)(x))

typedef struct local_time_offset_entry {
   unsigned char country_code1                          /*:8*/;
   unsigned char country_code2                          /*:8*/;
   unsigned char country_code3                          /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char country_region_id                      :6;
   unsigned char                                        :1;
   unsigned char local_time_offset_polarity             :1;
#else
   unsigned char local_time_offset_polarity             :1;
   unsigned char                                        :1;
   unsigned char country_region_id                      :6;
#endif
   unsigned char local_time_offset_h                    /*:8*/;
   unsigned char local_time_offset_m                    /*:8*/;
   unsigned char time_of_change_mjd_hi                  /*:8*/;
   unsigned char time_of_change_mjd_lo                  /*:8*/;
   unsigned char time_of_change_time_h                  /*:8*/;
   unsigned char time_of_change_time_m                  /*:8*/;
   unsigned char time_of_change_time_s                  /*:8*/;
   unsigned char next_time_offset_h                     /*:8*/;
   unsigned char next_time_offset_m                     /*:8*/;
} local_time_offset_entry_t ;
#define LOCAL_TIME_OFFSET_ENTRY_LEN sizeof (local_time_offset_entry_t)
#define CastLocalTimeOffsetEntry(x) ((local_time_offset_entry_t *)(x))

/* 0x59 subtitling_descriptor */
typedef struct descr_subtitling {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[]; /* item_subtitling */
} descr_subtitling_t;
#define DESCR_SUBTITLING_LEN sizeof (descr_subtitling_t)
#define CastSubtitlingDescriptor(x) ((descr_subtitling_t *)(x))

typedef struct item_subtitling {
   unsigned char lang_code1                             /*:8*/;
   unsigned char lang_code2                             /*:8*/;
   unsigned char lang_code3                             /*:8*/;
   unsigned char subtitling_type                        /*:8*/;
   unsigned char composition_page_id_hi                 /*:8*/;
   unsigned char composition_page_id_lo                 /*:8*/;
   unsigned char ancillary_page_id_hi                   /*:8*/;
   unsigned char ancillary_page_id_lo                   /*:8*/;
} item_subtitling_t;
#define ITEM_SUBTITLING_LEN sizeof (item_subtitling_t)
#define CastSubtitlingItem(x) ((item_subtitling_t *)(x))

/* 0x5A terrestrial_delivery_system_descriptor */
typedef struct descr_terrestrial_delivery {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char frequency1                             /*:8*/;
   unsigned char frequency2                             /*:8*/;
   unsigned char frequency3                             /*:8*/;
   unsigned char frequency4                             /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char bandwidth                              :3;
   unsigned char                                        :5;
#else
   unsigned char                                        :5;
   unsigned char bandwidth                              :3;
#endif
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char constellation                          :2;
   unsigned char hierarchy                              :3;
   unsigned char code_rate_HP                           :3;
#else
   unsigned char code_rate_HP                           :3;
   unsigned char hierarchy                              :3;
   unsigned char constellation                          :2;
#endif
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char code_rate_LP                           :3;
   unsigned char guard_interval                         :2;
   unsigned char transmission_mode                      :2;
   unsigned char other_frequency_flag                   :1;
#else
   unsigned char other_frequency_flag                   :1;
   unsigned char transmission_mode                      :2;
   unsigned char guard_interval                         :2;
   unsigned char code_rate_LP                           :3;
#endif
   unsigned char reserver2                              /*:8*/;
   unsigned char reserver3                              /*:8*/;
   unsigned char reserver4                              /*:8*/;
   unsigned char reserver5                              /*:8*/;
} descr_terrestrial_delivery_system_t;
#define DESCR_TERRESTRIAL_DELIVERY_SYSTEM_LEN sizeof (descr_terrestrial_delivery_system_t)
#define CastTerrestrialDeliverySystemDescriptor(x) ((descr_terrestrial_delivery_system_t *)(x))
#define GetTerrestrialDeliverySystemFrequency(x) HILO4(((descr_terrestrial_delivert_system_t *)x)->frequency)

/* 0x5B multilingual_network_name_descriptor */
typedef struct descr_multilingual_network_name {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[]; /* struct item_multilingual_network_name */
} descr_multilingual_network_name_t;
#define DESCR_MULTILINGUAL_NETWORK_NAME_LEN sizeof (descr_multilingual_network_name_t)
#define CastMultilingualNetworkNameDescriptor(x) ((descr_multilingual_network_name_t *)(x))

typedef struct item_multilingual_network_name {
   unsigned char lang_code1                           /*:8*/;
   unsigned char lang_code2                           /*:8*/;
   unsigned char lang_code3                           /*:8*/;
   unsigned char network_name_length                  /*:8*/;
   unsigned char network_name[];
} item_multilingual_network_name_t;

/* 0x5C multilingual_bouquet_name_descriptor */
typedef struct descr_multilingual_bouquet_name {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char names[]; /* struct item_multilingual_bouquet_name */
} descr_multilingual_bouquet_name_t;
#define DESCR_MULTILINGUAL_BOUQUET_NAME_LEN sizeof (descr_multilingual_bouquet_name_t)
#define CastMultilingualBouquetNameDescriptor(x) ((descr_multilingual_bouquet_name_t *)(x))

typedef struct item_multilingual_bouquet_name {
   unsigned char lang_code1                           /*:8*/;
   unsigned char lang_code2                           /*:8*/;
   unsigned char lang_code3                           /*:8*/;
   unsigned char bouquet_name_length                  /*:8*/;
   unsigned char bouquet_name[];
} item_multilingual_bouquet_name_t;

/* 0x5D multilingual_service_name_descriptor */
typedef struct descr_multilingual_service_name {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[]; /* struct multilingual_service_name_item */
} descr_multilingual_service_name_t;
#define DESCR_MULTILINGUAL_SERVICE_NAME_LEN sizeof (descr_multilingual_service_name_t)
#define CastMultilingualServiceNameDescriptor(x) ((descr_multilingual_service_name_t *)(x))

typedef struct multilingual_service_name_item {
   unsigned char lang_code1                           /*:8*/;
   unsigned char lang_code2                           /*:8*/;
   unsigned char lang_code3                           /*:8*/;
   unsigned char data[]; /* struct multilingual_service_name_item_(mid|end) */
} multilingual_service_name_item_t;
typedef struct multilingual_service_name_item_mid {
   unsigned char service_provider_name_length         /*:8*/;
   unsigned char service_provider_name[];
} multilingual_service_name_item_mid_t;
typedef struct multilingual_service_name_item_end {
   unsigned char service_name_length                  /*:8*/;
   unsigned char service_name[];
} multilingual_service_name_item_end_t;

/* 0x5E multilingual_component_descriptor */
typedef struct descr_multilingual_component {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char component_tag                          /*:8*/;
   unsigned char data[]; /* struct multilingual_component_item */
} descr_multilingual_component_t;
#define DESCR_MULTILINGUAL_COMPONENT_LEN sizeof (descr_multilingual_component_t)
#define CastMultilingualComponentDescriptor(x) ((descr_multilingual_component_t *)(x))

typedef struct item_multilingual_component {
   unsigned char lang_code1                             /*:8*/;
   unsigned char lang_code2                             /*:8*/;
   unsigned char lang_code3                             /*:8*/;
   unsigned char text_description_length                /*:8*/;
   unsigned char text_description[];
} item_multilingual_component_t;

/* 0x5F private_data_specifier_descriptor */
typedef struct descr_private_data_specifier {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char private_data_specifier1                /*:8*/;
   unsigned char private_data_specifier2                /*:8*/;
   unsigned char private_data_specifier3                /*:8*/;
   unsigned char private_data_specifier4                /*:8*/;
} descr_private_data_specifier_t;
#define DESCR_PRIVATE_DATA_SPECIFIER_LEN sizeof (descr_private_data_specifier_t)
#define CastPrivateDataSpecifierDescriptor(x) ((descr_private_data_specifier_t *)(x))
#define GetPrivateDataSpecifier(x) HILO4(((descr_private_data_specifier_t *)x)->private_data_specifier)

/* 0x60 service_move_descriptor */
typedef struct descr_service_move {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char new_original_network_id_hi             /*:8*/;
   unsigned char new_original_network_id_lo             /*:8*/;
   unsigned char new_transport_stream_id_hi             /*:8*/;
   unsigned char new_transport_stream_id_lo             /*:8*/;
   unsigned char new_service_id_hi                      /*:8*/;
   unsigned char new_service_id_lo                      /*:8*/;
} descr_service_move_t;
#define DESCR_SERVICE_MOVE_LEN sizeof (descr_service_move_t)
#define CastServiceMoveDescriptor(x) ((descr_service_move_t *)(x))

/* 0x61 short_smoothing_buffer_descriptor */
typedef struct descr_short_smoothing_buffer {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char sb_size                                :2;
   unsigned char sb_leak_rate                           :6;
#else
   unsigned char sb_leak_rate                           :6;
   unsigned char sb_size                                :2;
#endif
   unsigned char data[];
} descr_short_smoothing_buffer_t;
#define DESCR_SHORT_SMOOTHING_BUFFER_LEN sizeof (descr_short_smoothing_buffer_t)
#define CastShortSmoothingBufferDescriptor(x) ((descr_short_smoothing_buffer_t *)(x))

/* 0x62 frequency_list_descriptor */
typedef struct descr_frequency_list {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :6;
   unsigned char coding_type                            :2; /* 00=not def 01=satelite 10=cable 11=terrestrial */
#else
   unsigned char coding_type                            :2; /* 00=not def 01=satelite 10=cable 11=terrestrial */
   unsigned char                                        :6;
#endif
   unsigned char centre_frequency1                      /*:8*/;
   unsigned char centre_frequency2                      /*:8*/;
   unsigned char centre_frequency3                      /*:8*/;
   unsigned char centre_frequency4                      /*:8*/;
} descr_frequency_list_t;
#define DESCR_FREQUENCY_LIST_LEN sizeof (descr_frequency_list_t)
#define CastFrequencyListDescriptor(x) ((descr_frequency_list_t *)(x))

/* 0x63 partial_transport_stream_descriptor */
typedef struct descr_partial_transport_stream {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char peak_rate1                             :6;
#else
   unsigned char peak_rate1                             :6;
   unsigned char                                        :2;
#endif
   unsigned char peak_rate2                             /*:8*/;
   unsigned char peak_rate3                             /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char minimum_overall_smoothing_rate1        :6;
#else
   unsigned char minimum_overall_smoothing_rate1        :6;
   unsigned char                                        :2;
#endif
   unsigned char minimum_overall_smoothing_rate2        /*:8*/;
   unsigned char minimum_overall_smoothing_rate3        /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :2;
   unsigned char maximum_overall_smoothing_rate1        :6;
#else
   unsigned char maximum_overall_smoothing_rate1        :6;
   unsigned char                                        :2;
#endif
   unsigned char maximum_overall_smoothing_rate2        /*:8*/;
} descr_partial_transport_stream_t;
#define DESCR_PARTIAL_TRANSPORT_STREAM_LEN sizeof (descr_partial_transport_stream_t)
#define CastPartialDescriptor(x) ((descr_partial_transport_stream_t *)(x))
#define GetPartialTransportStreamCentreFrequency(x) HILO4(((descr_partial_transport_stream_t *)x)->centre_frequency)
#define GetPTSPeakRate(x) HILO2(((descr_partial_transport_stream *)x)->peak_rate)
#define GetPTSMinOSRate(x) HILO3(((descr_partial_transport_stream *)x)->minimum_overall_smoothing_rate)
#define GetPTSMaxOSRate(x) HILO2(((descr_partial_transport_stream *)x)->minimum_overall_smoothing_rate)

/* 0x64 data_broadcast_descriptor */
typedef struct descr_data_broadcast {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data_broadcast_id_hi                   /*:8*/;
   unsigned char data_broadcast_id_lo                   /*:8*/;
   unsigned char component_tag                          /*:8*/;
   unsigned char selector_length                        /*:8*/;
   unsigned char data[]; /* char[]; struct descr_data_broadcast_end */
} descr_data_broadcast_t;
typedef struct descr_data_broadcast_end {
   unsigned char lang_code1                             /*:8*/;
   unsigned char lang_code2                             /*:8*/;
   unsigned char lang_code3                             /*:8*/;
   unsigned char text_length                            /*:8*/;
   unsigned char text[];
} descr_data_broadcast_end_t;

/* 0x65 ca_system_descriptor */
typedef struct descr_ca_system {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[]; /* struct item_ca_system */
} descr_ca_system_t;
#define DESCR_CA_SYSTEM_LEN sizeof (descr_ca_system_t)
#define CastCaSystemDescriptor(x) ((descr_ca_system_t *)(x))

typedef struct item_ca_system {
   unsigned char CA_system_id_hi                        /*:8*/;
   unsigned char CA_system_id_lo                        /*:8*/;
} item_ca_system_t;

/* 0x66 data_broadcast_id_descriptor */
typedef struct descr_data_broadcast_id {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data_broadcast_id_hi                   /*:8*/;
   unsigned char data_broadcast_id_lo                   /*:8*/;
} descr_data_broadcast_id_t;
#define DESCR_DATA_BROADCAST_ID_LEN sizeof (descr_data_broadcast_id_t)
#define CastDataBroadcastIdDescriptor(x) ((descr_data_broadcast_id_t *)(x))

/* 0x67 transport_stream_descriptor */
typedef struct descr_transport_stream {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   /* TBD */
} descr_transport_stream_t;
#define DESCR_TRANSPORT_STREAM_LEN sizeof (descr_transport_stream_t)
#define CastTransportStreamDescriptor(x) ((descr_transport_stream_t *)(x))

/* 0x68 dsng_descriptor */
typedef struct descr_dsng {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   /* TBD */
} descr_dsng_t;
#define DESCR_DSNG_LEN sizeof (descr_dsng_t)
#define CastDsngDescriptor(x) ((descr_dsng_t *)(x))

/* 0x69 programme_identificaion_label_descriptor */
typedef struct descr_pdc {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char                                        :4;
   unsigned char day                                    :5;
   unsigned char month                                  :4;
   unsigned char hour                                   :5;
   unsigned char minute                                 :6;
#else
   unsigned char minute                                 :6;
   unsigned char hour                                   :5;
   unsigned char month                                  :4;
   unsigned char day                                    :5;
   unsigned char                                        :4;
#endif
} descr_pdc_t;
#define DESCR_PDC_LEN sizeof (descr_pdc_t)
#define CastPdcDescriptor(x) ((descr_pdc_t *)(x))

/* 0x6A ac3_descriptor */
typedef struct descr_ac3 {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char ac3_type_flag                          :1;
   unsigned char bsid_flag                              :1;
   unsigned char mainid_flag                            :1;
   unsigned char asvc_flag                              :1;
   unsigned char                                        :4;
#else
   unsigned char                                        :4;
   unsigned char asvc_flag                              :1;
   unsigned char mainid_flag                            :1;
   unsigned char bsid_flag                              :1;
   unsigned char ac3_type_flag                          :1;
#endif
   unsigned char ac3_type                               /*:8*/;
   unsigned char bsid                                   /*:8*/;
   unsigned char mainid                                 /*:8*/;
   unsigned char asvc                                   /*:8*/;
} descr_ac3_t;
#define DESCR_AC3_LEN sizeof (descr_ac3_t)
#define CastAc3Descriptor(x) ((descr_ac3_t *)(x))

/* 0x6B ancillary_data_descriptor */
typedef struct descr_ancillary_data {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char ancillary_data_identifier              /*:8*/;
} descr_ancillary_data_t;
#define DESCR_ANCILLARY_DATA_LEN sizeof (descr_ancillary_data_t)
#define CastAncillaryDataDescriptor(x) ((descr_ancillary_data_t *)(x))

/* 0x6C cell_list_descriptor */
typedef struct descr_cell_list {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   /* TBD */
} descr_cell_list_t;
#define DESCR_CELL_LIST_LEN sizeof (descr_cell_list_t)
#define CastCellListDescriptor(x) ((descr_cell_list_t *)(x))

/* 0x6D cell_frequency_link_descriptor */
typedef struct descr_cell_frequency_link {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   /* TBD */
} descr_cell_frequency_link_t;
#define DESCR_CELL_FREQUENCY_LINK_LEN sizeof (descr_cell_frequency_link_t)
#define CastCellFrequencyLinkDescriptor(x) ((descr_cell_frequency_link_t *)(x))

/* 0x6E announcement_support_descriptor */
typedef struct descr_announcement_support {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   /* TBD */
} descr_announcement_support_t;
#define DESCR_ANNOUNCEMENT_SUPPORT_LEN sizeof (descr_announcement_support_t)
#define CastAnnouncementSupportDescriptor(x) ((descr_announcement_support_t *)(x))

/* 0x73 default_authority_descriptor */
typedef struct default_authority {
	unsigned char descriptor_tag;
	unsigned char descriptor_length;
	unsigned char data[];
} descr_default_authority_t;

/* 0x76 content_identifier_descriptor */
typedef struct descr_content_identifier {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char data[];
} descr_content_identifier_t;

typedef struct descr_content_identifier_crid {
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char crid_type                             :6;
   unsigned char crid_location                         :2;
#else
   unsigned char crid_location                         :2;
   unsigned char crid_type                             :6;
#endif
   unsigned char crid_ref_data[];
} descr_content_identifier_crid_t;

typedef struct descr_content_identifier_crid_local {
   unsigned char crid_length                           /*:8*/;
   unsigned char crid_byte[];
} descr_content_identifier_crid_local_t;


/* 0x80 custom_category_descriptor TODO */
typedef struct descr_custom_category {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char dummy                                  /*:8*/; // 7F
   unsigned char data_length                            /*:8*/;
   unsigned char data[]; /* struct custom_category_item(1|2) */
} descr_custom_category_t;
struct custom_category_item1 {
   unsigned char dummy                                  /*:8*/; // 10 40
};
struct custom_category_item2 {
   unsigned char length                                 /*:8*/;
   unsigned char data[]; /* struct custom_category_item3 */
};
struct custom_category_item3 {
   unsigned char dummy0                                 /*:8*/; // FF
#if BYTE_ORDER == BIG_ENDIAN
   unsigned char content_nibble_level_1                 :4;
   unsigned char content_nibble_level_2                 :4;
   unsigned char user_nibble_1                          :4;
   unsigned char user_nibble_2                          :4;
#else
   unsigned char user_nibble_2                          :4;
   unsigned char user_nibble_1                          :4;
   unsigned char content_nibble_level_2                 :4;
   unsigned char content_nibble_level_1                 :4;
#endif
   unsigned char dummy1[2]                              /*:8*/; // CF E2 | D4 48
   unsigned char text_length                            /*:8*/;
   unsigned char text[];
};

/* 0x81 xxx_descriptor TODO */
typedef struct descr_xxx {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char dummy                                  /*:8 FF*/;
} descr_xxx_t;
#define DESCR_XXX_LEN sizeof (descr_xxx_t)
#define CastXxxDescriptor(x) ((descr_xxx_t *)(x))

/* 0x82 vps_descriptor TODO */
typedef struct descr_vps {
   unsigned char descriptor_tag                         /*:8*/;
   unsigned char descriptor_length                      /*:8*/;
   unsigned char hour[2]                                /*:8*/;
   unsigned char delimiter_time                         /*:8 ':'*/;
   unsigned char minute[2]                              /*:8*/;
   unsigned char day[2]                                 /*:8*/;
   unsigned char delimiter_date                         /*:8 '.'*/;
   unsigned char month[2]                               /*:8*/;
   unsigned char delimiter                              /*:8 '#'*/;
   unsigned char number[2]                              /*:8*/;
} descr_vps_t;
#define DESCR_VPS_LEN sizeof (descr_vps_t)
#define CastVpsDescriptor(x) ((descr_vps_t *)(x))

/* 0x83 logical_channel_descriptor */
typedef struct descr_logical_channel {
	unsigned char descriptor_tag;
	unsigned char descriptor_length;
	unsigned char data[];
} descr_logical_channel_t;
#define DESCR_LC_LEN sizeof(descr_logical_channel_t)

typedef struct descr_logical_channel_svc {
	unsigned char service_id_hi;
	unsigned char service_id_lo;
	unsigned char logical_channel_number_hi              :2;
	unsigned char reserved                               :5;
	unsigned char visible_service_flag                   :1;
	unsigned char logical_channel_number_lo;
} descr_logical_channel_svc_t;
#define DESCR_LCSVC_LEN sizeof(descr_logical_channel_svc_t)
