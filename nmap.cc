#include "./multilang/zh_CN/nmap.h"
/***************************************************************************
 * nmap.cc -- Currently handles some of Nmap's port scanning features as   *
 * well as the command line user interface.  Note that the actual main()   *
 * function is in main.cc                                                  *
 *                                                                         *
 ***********************IMPORTANT NMAP LICENSE TERMS************************
 *                                                                         *
 * The Nmap Security Scanner is (C) 1996-2022 Nmap Software LLC ("The Nmap *
 * Project"). Nmap is also a registered trademark of the Nmap Project.     *
 *                                                                         *
 * This program is distributed under the terms of the Nmap Public Source   *
 * License (NPSL). The exact license text applying to a particular Nmap    *
 * release or source code control revision is contained in the LICENSE     *
 * file distributed with that version of Nmap or source code control       *
 * revision. More Nmap copyright/legal information is available from       *
 * https://nmap.org/book/man-legal.html, and further information on the    *
 * NPSL license itself can be found at https://nmap.org/npsl/ . This       *
 * header summarizes some key points from the Nmap license, but is no      *
 * substitute for the actual license text.                                 *
 *                                                                         *
 * Nmap is generally free for end users to download and use themselves,    *
 * including commercial use. It is available from https://nmap.org.        *
 *                                                                         *
 * The Nmap license generally prohibits companies from using and           *
 * redistributing Nmap in commercial products, but we sell a special Nmap  *
 * OEM Edition with a more permissive license and special features for     *
 * this purpose. See https://nmap.org/oem/                                 *
 *                                                                         *
 * If you have received a written Nmap license agreement or contract       *
 * stating terms other than these (such as an Nmap OEM license), you may   *
 * choose to use and redistribute Nmap under those terms instead.          *
 *                                                                         *
 * The official Nmap Windows builds include the Npcap software             *
 * (https://npcap.com) for packet capture and transmission. It is under    *
 * separate license terms which forbid redistribution without special      *
 * permission. So the official Nmap Windows builds may not be              *
 * redistributed without special permission (such as an Nmap OEM           *
 * license).                                                               *
 *                                                                         *
 * Source is provided to this software because we believe users have a     *
 * right to know exactly what a program is going to do before they run it. *
 * This also allows you to audit the software for security holes.          *
 *                                                                         *
 * Source code also allows you to port Nmap to new platforms, fix bugs,    *
 * and add new features.  You are highly encouraged to submit your         *
 * changes as a Github PR or by email to the dev@nmap.org mailing list     *
 * for possible incorporation into the main distribution. Unless you       *
 * specify otherwise, it is understood that you are offering us very       *
 * broad rights to use your submissions as described in the Nmap Public    *
 * Source License Contributor Agreement. This is important because we      *
 * fund the project by selling licenses with various terms, and also       *
 * because the inability to relicense code has caused devastating          *
 * problems for other Free Software projects (such as KDE and NASM).       *
 *                                                                         *
 * The free version of Nmap is distributed in the hope that it will be     *
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. Warranties,        *
 * indemnification and commercial support are all available through the    *
 * Npcap OEM program--see https://nmap.org/oem/                            *
 *                                                                         *
 ***************************************************************************/

/* $Id$ */

#ifdef WIN32
#include "winfix.h"
/* This name collides in the following include. */
#undef PS_NONE
#include <shlobj.h>
#endif

#include "nmap.h"
#include "osscan.h"
#include "scan_engine.h"
#include "FPEngine.h"
#include "idle_scan.h"
#include "NmapOps.h"
#include "MACLookup.h"
#include "traceroute.h"
#include "nmap_tty.h"
#include "nmap_ftp.h"
#include "services.h"
#include "targets.h"
#include "tcpip.h"
#include "NewTargets.h"
#include "Target.h"
#include "service_scan.h"
#include "charpool.h"
#include "nmap_error.h"
#include "utils.h"
#include "xml.h"
#include "scan_lists.h"
#include "payload.h"

#ifndef NOLUA
#include "nse_main.h"
#endif

#ifdef HAVE_SIGNAL
#include <signal.h>
#endif

#include <fcntl.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifndef IPPROTO_SCTP
#include "libnetutil/netutil.h"
#endif

#if HAVE_OPENSSL
#include <openssl/opensslv.h>
#include <openssl/crypto.h>
#endif

#if HAVE_LIBSSH2
#include <libssh2.h>
#endif

#if HAVE_LIBZ
#include <zlib.h>
#endif

/* To get the version number only. */
#ifdef WIN32
#include "libdnet-stripped/include/dnet_winconfig.h"
#else
#include "libdnet-stripped/include/config.h"
#endif
#define DNET_VERSION VERSION

#ifdef LINUX
/* Check for Windows Subsystem for Linux (WSL) */
#include <sys/utsname.h>
#endif

#include <string>
#include <sstream>
#include <vector>

/* global options */
extern char *optarg;
extern int optind;
extern NmapOps o;  /* option structure */

static void display_nmap_version();

/* A mechanism to save argv[0] for code that requires that. */
static const char *program_name = NULL;

void set_program_name(const char *name) {
  program_name = name;
}

static const char *get_program_name(void) {
  return program_name;
}

/* parse the --scanflags argument.  It can be a number >=0 or a string consisting of TCP flag names like "URGPSHFIN".  Returns -1 if the argument is invalid. */
static int parse_scanflags(char *arg) {
  int flagval = 0;
  char *end = NULL;

  if (isdigit((int) (unsigned char) arg[0])) {
    flagval = strtol(arg, &end, 0);
    if (*end || flagval < 0 || flagval > 255)
      return -1;
  } else {
    if (strcasestr(arg, str_FIN_3_FIN_69611))
      flagval |= TH_FIN;
    if (strcasestr(arg, str_SYN_3_SYN_82600))
      flagval |= TH_SYN;
    if (strcasestr(arg, str_RST_3_RST_81459) || strcasestr(arg, str_RESET_5_RESET_77866287))
      flagval |= TH_RST;
    if (strcasestr(arg, str_PSH_3_PSH_79525) || strcasestr(arg, str_PUSH_4_PUSH_2467610))
      flagval |= TH_PUSH;
    if (strcasestr(arg, str_ACK_3_ACK_64617))
      flagval |= TH_ACK;
    if (strcasestr(arg, str_URG_3_URG_84298))
      flagval |= TH_URG;
    if (strcasestr(arg, str_ECE_3_ECE_68455))
      flagval |= TH_ECE;
    if (strcasestr(arg, str_CWR_3_CWR_67166))
      flagval |= TH_CWR;
    if (strcasestr(arg, str_ALL_3_ALL_64897))
      flagval = 255;
    if (strcasestr(arg, str_NONE_4_NONE_2402104))
      flagval = 0;
  }
  return flagval;
}

static void printusage() {

  printf(str_37s32_14_4192n_1997646523
         str_Usage_61_2592n_1231433074
         str_TARGE_23_5892n_1589947295
         str_3232C_52_4692n_1010345248
         str_3232E_72_5492n_1966309877
         str_32324_58_ks92n_1478905843
         str_32324_42_ts92n_292323120
         str_32324_65_ks92n_411345408
         str_32324_56_le92n_2145651237
         str_HOST3_17_5892n_2084494039
         str_32324_48_an92n_286956782
         str_32324_38_an92n_2017434118
         str_32324_57_ry92n_1943342
         str_32324_77_ts92n_1183670211
         str_32324_73_es92n_189480285
         str_32324_40_ng92n_15987073
         str_32324_70_9392n_1198820794
         str_32324_65_rs92n_630661443
         str_32324_39_er92n_1580194044
         str_32324_45_st92n_1436703483
         str_SCAN3_18_5892n_633636755
         str_32324_62_ns92n_438386765
         str_32324_17_an92n_1241803589
         str_32324_44_ns92n_715279005
         str_32324_49_gs92n_1953726934
         str_32324_44_an92n_880940338
         str_32324_39_ns92n_479286505
         str_32324_25_an92n_1064850466
         str_32324_40_an92n_1715604543
         str_PORT3_36_5892n_1760119084
         str_32324_47_ts92n_867746835
         str_32323_66_8992n_724148582
         str_32324_76_ng92n_1753742110
         str_32324_58_an92n_2100935111
         str_32324_49_ze92n_471341561
         str_32324_57_ts92n_250358139
         str_32324_61_6292n_1786077364
         str_SERVI_28_5892n_1128473172
         str_32324_59_fo92n_1947526189
         str_32324_73_4192n_1841779415
         str_32324_62_4192n_1595872032
         str_32324_55_4192n_358970249
         str_32324_72_4192n_1382170674
#ifndef NOLUA
         str_SCRIP_14_5892n_144542059
         str_32324_39_lt92n_763688496
         str_32324_70_of92n_40574829
         str_32323_59_es92n_1282776714
         str_32324_67_ts92n_2017898244
         str_32324_66_le92n_389508044
         str_32324_51_ed92n_1046336231
         str_32324_50_4692n_2132646971
         str_32324_57_4692n_2012937130
         str_32323_71_or92n_1855939615
         str_32323_31_4692n_1458430102
#endif
         str_OS32D_15_5892n_1295565232
         str_32324_27_on92n_1352535111
         str_32324_59_ts92n_1454647854
         str_32324_46_ly92n_282322894
         str_TIMIN_25_5892n_138772817
         str_3232O_76_4492n_320944270
         str_32323_73_4692n_1358614597
         str_32324_51_4192n_935469462
         str_32324_72_es92n_217280263
         str_32324_72_on92n_27162692
         str_32324_75_es92n_677715995
         str_32323_30_4692n_835663946
         str_32324_74_4692n_1400155631
         str_32324_60_ng92n_1060458805
         str_32324_69_es92n_27692751
         str_32324_72_nd92n_1194082816
         str_32324_72_nd92n_1968819703
         str_FIREW_36_5892n_223705715
         str_32324_62_4192n_1682932523
         str_32324_57_ys92n_665714811
         str_32324_41_ss92n_1302792639
         str_32324_39_ce92n_1460386474
         str_32324_53_er92n_44206870
         str_32324_78_es92n_1424803016
         str_32324_64_ts92n_490630295
         str_32324_72_ts92n_738994472
         str_32324_59_ts92n_792689767
         str_32324_66_ns92n_557944367
         str_32324_42_ld92n_1328878973
         str_32324_72_ss92n_1238722271
         str_32324_61_um92n_1643372752
         str_OUTPU_9_5892n_1318460049
         str_32324_71_4492n_1826062741
         str_32323_64_4692n_912059112
         str_32324_61_ce92n_315748810
         str_32324_69_4192n_997613655
         str_32324_69_4192n_200982228
         str_32324_64_te92n_648838589
         str_32324_51_ts92n_1441436775
         str_32324_54_ed92n_418702823
         str_32324_62_4192n_1643294234
         str_32324_73_es92n_886832549
         str_32324_47_an92n_521131501
         str_32324_63_rd92n_350193080
         str_32324_75_ML92n_325108916
         str_32324_70_ML92n_129323164
         str_32324_71_ut92n_1336014630
         str_MISC5_7_5892n_741718650
         str_32324_28_ng92n_284081629
         str_32324_79_te92n_867189635
         str_32324_63_on92n_528708505
         str_32324_70_ts92n_675461663
         str_32324_58_ed92n_207991478
         str_32324_63_es92n_880120718
         str_32324_28_er92n_1657849096
         str_32324_37_4692n_85028355
         str_EXAMP_11_5892n_897280073
         str_3232n_30_rg92n_755212334
         str_3232n_41_7892n_162874813
         str_3232n_31_8092n_762693291
         str_SEE32_81_ES92n_1633690106, NMAP_NAME, NMAP_VERSION, NMAP_URL);
}

#ifdef WIN32
static void check_setugid(void) {
}
#else
/* Show a warning when running setuid or setgid, as this allows code execution
   (for example NSE scripts) as the owner/group. */
static void check_setugid(void) {
  if (getuid() != geteuid())
    error(str_WARNI_75_4692n_1236354962);
  if (getgid() != getegid())
    error(str_WARNI_75_4692n_1685421280);
}
#endif

static void insert_port_into_merge_list(unsigned short *mlist,
                                        int *merged_port_count,
                                        unsigned short p) {
  int i;
  // make sure the port isn't already in the list
  for (i = 0; i < *merged_port_count; i++) {
    if (mlist[i] == p) {
      return;
    }
  }
  mlist[*merged_port_count] = p;
  (*merged_port_count)++;
}

static unsigned short *merge_port_lists(unsigned short *port_list1, int count1,
                                        unsigned short *port_list2, int count2,
                                        int *merged_port_count) {
  int i;
  unsigned short *merged_port_list = NULL;

  *merged_port_count = 0;

  merged_port_list =
    (unsigned short *) safe_zalloc((count1 + count2) * sizeof(unsigned short));

  for (i = 0; i < count1; i++) {
    insert_port_into_merge_list(merged_port_list,
                                merged_port_count,
                                port_list1[i]);
  }
  for (i = 0; i < count2; i++) {
    insert_port_into_merge_list(merged_port_list,
                                merged_port_count,
                                port_list2[i]);
  }

  // if there were duplicate ports then we can save some memory
  if (*merged_port_count < (count1 + count2)) {
    merged_port_list = (unsigned short*)
                       safe_realloc(merged_port_list,
                                    (*merged_port_count) * sizeof(unsigned short));
  }

  return merged_port_list;
}

void validate_scan_lists(scan_lists &vports, NmapOps &vo) {
  if (vo.pingtype == PINGTYPE_UNKNOWN) {
    if (vo.isr00t) {
      if (vo.pf() == PF_INET) {
        vo.pingtype = DEFAULT_IPV4_PING_TYPES;
      } else {
        vo.pingtype = DEFAULT_IPV6_PING_TYPES;
      }
      getpts_simple(DEFAULT_PING_ACK_PORT_SPEC, SCAN_TCP_PORT,
                    &vports.ack_ping_ports, &vports.ack_ping_count);
      getpts_simple(DEFAULT_PING_SYN_PORT_SPEC, SCAN_TCP_PORT,
                    &vports.syn_ping_ports, &vports.syn_ping_count);
    } else {
      vo.pingtype = PINGTYPE_TCP; // if nonr00t
      getpts_simple(DEFAULT_PING_CONNECT_PORT_SPEC, SCAN_TCP_PORT,
                    &vports.syn_ping_ports, &vports.syn_ping_count);
    }
  }

  if ((vo.pingtype & PINGTYPE_TCP) && (!vo.isr00t)) {
    // We will have to do a connect() style ping
    // Pretend we wanted SYN probes all along.
    if (vports.ack_ping_count > 0) {
      // Combine the ACK and SYN ping port lists since they both reduce to
      // SYN probes in this case
      unsigned short *merged_port_list;
      int merged_port_count;

      merged_port_list = merge_port_lists(
                           vports.syn_ping_ports, vports.syn_ping_count,
                           vports.ack_ping_ports, vports.ack_ping_count,
                           &merged_port_count);

      // clean up a bit
      free(vports.syn_ping_ports);
      free(vports.ack_ping_ports);

      vports.syn_ping_count = merged_port_count;
      vports.syn_ping_ports = merged_port_list;
      vports.ack_ping_count = 0;
      vports.ack_ping_ports = NULL;
    }
    vo.pingtype &= ~PINGTYPE_TCP_USE_ACK;
    vo.pingtype |= PINGTYPE_TCP_USE_SYN;
  }

  if (!vo.isr00t) {
    if (vo.pingtype & (PINGTYPE_ICMP_PING | PINGTYPE_ICMP_MASK | PINGTYPE_ICMP_TS)) {
      error(str_Warni_65_2ICMP_266543879);
      vo.pingtype = PINGTYPE_TCP;
      if (vports.syn_ping_count == 0) {
        getpts_simple(DEFAULT_TCP_PROBE_PORT_SPEC, SCAN_TCP_PORT, &vports.syn_ping_ports, &vports.syn_ping_count);
        assert(vports.syn_ping_count > 0);
      }
    }
  }
}

struct ftpinfo ftp = get_default_ftpinfo();

/* A list of targets to be displayed by the --route-dst debugging option. */
static std::vector<std::string> route_dst_hosts;

struct scan_lists ports = { 0 };

/* This struct is used is a temporary storage place that holds options that
   can't be correctly parsed and interpreted before the entire command line has
   been read. Examples are -6 and -S. Trying to set the source address without
   knowing the address family first could result in a failure if you pass an
   IPv6 address and the address family is still IPv4. */
static struct delayed_options {
public:
  delayed_options() {
    this->pre_max_parallelism   = -1;
    this->pre_scan_delay        = -1;
    this->pre_max_scan_delay    = -1;
    this->pre_init_rtt_timeout  = -1;
    this->pre_min_rtt_timeout   = -1;
    this->pre_max_rtt_timeout   = -1;
    this->pre_max_retries       = -1;
    this->pre_host_timeout      = -1;
#ifndef NOLUA
    this->pre_scripttimeout     = -1;
#endif
    this->iflist                = false;
    this->advanced              = false;
    this->af                    = AF_UNSPEC;
    this->decoys                = false;
    this->raw_scan_options      = false;
  }

  // Pre-specified timing parameters.
  // These are stored here during the parsing of the arguments so that we can
  // set the defaults specified by any timing template options (-T2, etc) BEFORE
  // any of these. In other words, these always take precedence over the templates.
  int   pre_max_parallelism, pre_scan_delay, pre_max_scan_delay;
  int   pre_init_rtt_timeout, pre_min_rtt_timeout, pre_max_rtt_timeout;
  int   pre_max_retries;
  long  pre_host_timeout;
#ifndef NOLUA
  double pre_scripttimeout;
#endif
  char  *machinefilename, *kiddiefilename, *normalfilename, *xmlfilename;
  bool  iflist, decoys, advanced, raw_scan_options;
  char  *exclude_spec, *exclude_file;
  char  *spoofSource, *decoy_arguments;
  const char *spoofmac;
  int af;
  std::vector<std::string> verbose_out;

  void warn_deprecated (const char *given, const char *replacement) {
    std::ostringstream os;
    os << str_Warni_14_e3245_811811469 << given << str_32opt_35_e3245_739363086 << replacement;
    this->verbose_out.push_back(os.str());
  }

} delayed_options;

struct tm local_time;

static void test_file_name(const char *filename, const char *option) {
  if (filename[0] == '-' && filename[1] != '\0') {
    fatal(str_Outpu_90_uch46_1783155185, option, filename);
  } else if (strcmp(option, str_o_1_o_111) == 0 && strchr(str_NAXGS_5_NAXGS_74057905, filename[0])) {
    fatal(str_You32_75_3237s_1080535085, filename[0], filename + 1);
  } else if (filename[0] == '-' && strcmp(option,str_oA_2_oA_3506) == 0) {
    fatal(str_Canno_47_out46_1668451089);
  }
}

void parse_options(int argc, char **argv) {
  char *p;
  int arg;
  long l;
  double d;
  char *endptr = NULL;
  char errstr[256];
  int option_index;
#ifdef WORDS_BIGENDIAN
  int k[]={2037345391,1935892846,0,1279608146,1331241034,1162758985,1314070817,554303488,1869291630,1768383852};
#else
  int k[]={1869377401,1851876211,0,1380271436,1243633999,1229672005,555832142,2593,1847618415,1818584937};
#endif

  struct option long_options[] = {
    {str_versi_7_rsion_351608024, no_argument, 0, 'V'},
    {str_verbo_7_rbose_351107458, no_argument, 0, 'v'},
    {str_datad_7_tadir_1443200163, required_argument, 0, 0},
    {str_servi_9_icedb_194184717, required_argument, 0, 0},
    {str_versi_9_iondb_1407102122, required_argument, 0, 0},
    {str_debug_5_debug_95458899, optional_argument, 0, 'd'},
    {str_help_4_help_3198785, no_argument, 0, 'h'},
    {str_iflis_6_flist_1191385285, no_argument, 0, 0},
    {str_relea_14_emory_1013075913, no_argument, 0, 0},
    {str_nogcc_5_nogcc_104996262, no_argument, 0, 0},
    {str_max45_12_tries_195217935, required_argument, 0, 0},
    {str_max45_15_elism_662021399, required_argument, 0, 'M'},
    {str_min45_15_elism_598152937, required_argument, 0, 0},
    {str_timin_6_iming_873664438, required_argument, 0, 'T'},
    {str_max45_15_meout_416791795, required_argument, 0, 0},
    {str_min45_15_meout_1563130527, required_argument, 0, 0},
    {str_initi_19_meout_1087723821, required_argument, 0, 0},
    {str_exclu_11_efile_1203394134, required_argument, 0, 0},
    {str_exclu_7_clude_1321148966, required_argument, 0, 0},
    {str_max45_13_group_843982702, required_argument, 0, 0},
    {str_min45_13_group_187066652, required_argument, 0, 0},
    {str_open_4_open_3417674, no_argument, 0, 0},
    {str_scanf_9_flags_1819038166, required_argument, 0, 0},
    {str_defea_20_limit_2096725307, no_argument, 0, 0},
    {str_defea_21_limit_1982538839, no_argument, 0, 0},
    {str_host4_12_meout_544803608, required_argument, 0, 0},
    {str_scan4_10_delay_1253572229, required_argument, 0, 0},
    {str_max45_14_delay_1280428672, required_argument, 0, 0},
    {str_max45_11_tries_347400897, required_argument, 0, 0},
    {str_oA_2_oA_3506, required_argument, 0, 0},
    {str_oN_2_oN_3519, required_argument, 0, 0},
    {str_oM_2_oM_3518, required_argument, 0, 0},
    {str_oG_2_oG_3512, required_argument, 0, 0},
    {str_oS_2_oS_3524, required_argument, 0, 0},
    {str_oH_2_oH_3513, required_argument, 0, 0},
    {str_oX_2_oX_3529, required_argument, 0, 0},
    {str_iL_2_iL_3331, required_argument, 0, 0},
    {str_iR_2_iR_3337, required_argument, 0, 0},
    {str_sI_2_sI_3638, required_argument, 0, 0},
    {str_sourc_11_5port_382082653, required_argument, 0, 'g'},
    {str_rando_15_hosts_1756344089, no_argument, 0, 0},
    {str_nsock_12_ngine_1320937675, required_argument, 0, 0},
    {str_proxi_7_oxies_308889716, required_argument, 0, 0},
    {str_proxy_5_proxy_106941038, required_argument, 0, 0},
    {str_disco_20_45rst_556583729, no_argument, 0, 0},
    {str_ossca_12_limit_717728857, no_argument, 0, 0}, /* skip OSScan if no open ports */
    {str_ossca_12_guess_713461365, no_argument, 0, 0}, /* More guessing flexibility */
    {str_fuzzy_5_fuzzy_97805834, no_argument, 0, 0}, /* Alias for osscan_guess */
    {str_packe_12_trace_1550131748, no_argument, 0, 0}, /* Display all packets sent/rcv */
    {str_versi_13_trace_1004661876, no_argument, 0, 0}, /* Display -sV related activity */
    {str_data_4_data_3076010, required_argument, 0, 0},
    {str_data4_11_tring_595486588, required_argument, 0, 0},
    {str_data4_11_ength_381108817, required_argument, 0, 0},
    {str_send4_8_45eth_14873360, no_argument, 0, 0},
    {str_send4_7_d45ip_1246446320, no_argument, 0, 0},
    {str_style_10_sheet_158213710, required_argument, 0, 0},
    {str_no45s_13_sheet_453430320, no_argument, 0, 0},
    {str_webxm_6_ebxml_791784381, no_argument, 0, 0},
    {str_rH_2_rH_3606, no_argument, 0, 0},
    {str_vv_2_vv_3776, no_argument, 0, 0},
    {str_ff_2_ff_3264, no_argument, 0, 0},
    {str_privi_10_leged_1654399021, no_argument, 0, 0},
    {str_unpri_12_leged_467223340, no_argument, 0, 0},
    {str_mtu_3_mtu_108462, required_argument, 0, 0},
    {str_appen_13_utput_1106145316, no_argument, 0, 0},
    {str_nonin_14_ctive_878465525, no_argument, 0, 0},
    {str_spoof_9_45mac_1181309381, required_argument, 0, 0},
    {str_thc_3_thc_114799, no_argument, 0, 0},
    {str_badsu_6_adsum_1396635610, no_argument, 0, 0},
    {str_ttl_3_ttl_115180, required_argument, 0, 0}, /* Time to live */
    {str_trace_10_route_1005569060, no_argument, 0, 0},
    {str_reaso_6_eason_934964668, no_argument, 0, 0},
    {str_allpo_8_ports_1813830417, no_argument, 0, 0},
    {str_versi_17_nsity_1957642502, required_argument, 0, 0},
    {str_versi_13_light_1012312227, no_argument, 0, 0},
    {str_versi_11_45all_144080472, no_argument, 0, 0},
    {str_syste_10_45dns_1576733127, no_argument, 0, 0},
    {str_resol_11_45all_376318228, no_argument, 0, 0},
    {str_uniqu_6_nique_840528943, no_argument, 0, 0},
    {str_log45_10_rrors_605614192, no_argument, 0, 0},
    {str_depre_22_class_468379728, no_argument, 0, 0},
    {(char*)k, no_argument, 0, 0},
    {str_dns45_11_rvers_831534874, required_argument, 0, 0},
    {str_port4_10_ratio_1209087625, required_argument, 0, 0},
    {str_exclu_13_ports_1607471895, required_argument, 0, 0},
    {str_top45_9_ports_1928700100, required_argument, 0, 0},
#ifndef NOLUA
    {str_scrip_6_cript_907685685, required_argument, 0, 0},
    {str_scrip_12_trace_1370777543, no_argument, 0, 0},
    {str_scrip_15_atedb_232342637, no_argument, 0, 0},
    {str_scrip_11_5args_1756330857, required_argument, 0, 0},
    {str_scrip_16_5file_1775477722, required_argument, 0, 0},
    {str_scrip_11_5help_1756527053, required_argument, 0, 0},
    {str_scrip_14_meout_991233877, required_argument, 0, 0},
#endif
    {str_ip45o_10_tions_1190800310, required_argument, 0, 0},
    {str_min45_8_5rate_750236749, required_argument, 0, 0},
    {str_max45_8_5rate_1522715323, required_argument, 0, 0},
    {str_adler_7_ler32_1140680715, no_argument, 0, 0},
    {str_stats_11_every_1952903099, required_argument, 0, 0},
    {str_disab_16_5ping_554208585, no_argument, 0, 0},
    {str_route_9_45dst_871151355, required_argument, 0, 0},
    {str_resum_6_esume_934426579, required_argument, 0, 0},
    {0, 0, 0, 0}
  };

  /* Users have trouble with editors munging ascii hyphens into any of various
   * dashes. We'll check that none of these is in the command-line first: */
  for (arg=1; arg < argc; arg++) {
    // Just look at the first character of each.
    switch(argv[arg][0]) {
      case '\xe2': // UTF-8, have to look farther
        // U+2010 through U+2015 are the most likely
        if (argv[arg][1] != '\x80'
            || argv[arg][2] < '\x90'
            || argv[arg][2] > '\x95')
          break;
      case '\x96': // Windows 12** en dash
      case '\x97': // Windows 12** em dash
        fatal(str_Unpar_49_3237d_908355399, arg);
      default:
        break;
    }
  }

  /* OK, lets parse these args! */
  optind = 1; /* so it can be called multiple times */
  while ((arg = getopt_long_only(argc, argv, str_46Ab5_49_v5858_156932431, long_options, &option_index)) != EOF) {
    switch (arg) {
    case 0:
#ifndef NOLUA
      if (strcmp(long_options[option_index].name, str_scrip_6_cript_907685685) == 0) {
        o.script = true;
        o.chooseScripts(optarg);
      } else if (strcmp(long_options[option_index].name, str_scrip_11_5args_1756330857) == 0) {
        o.scriptargs = strdup(optarg);
      } else if (strcmp(long_options[option_index].name, str_scrip_16_5file_1775477722) == 0) {
        o.scriptargsfile = strdup(optarg);
      } else if (strcmp(long_options[option_index].name, str_scrip_12_trace_1370777543) == 0) {
        o.scripttrace = true;
      } else if (strcmp(long_options[option_index].name, str_scrip_15_atedb_232342637) == 0) {
        o.scriptupdatedb = true;
      } else if (strcmp(long_options[option_index].name, str_scrip_11_5help_1756527053) == 0) {
        o.scripthelp = true;
        o.chooseScripts(optarg);
      } else if (strcmp(long_options[option_index].name, str_scrip_14_meout_991233877) == 0) {
        d = tval2secs(optarg);
        if (d < 0 || d > LONG_MAX)
          fatal(str_Bogus_41_ified_1870406969);
        delayed_options.pre_scripttimeout = d;
      } else
#endif
        if (strcmp(long_options[option_index].name, str_max45_12_tries_195217935) == 0) {
          l = atoi(optarg);
          if (l < 1 || l > 50)
            fatal(str_Bogus_77_ive41_1583525540);
          o.setMaxOSTries(l);
        } else if (strcmp(long_options[option_index].name, str_max45_15_meout_416791795) == 0) {
          l = tval2msecs(optarg);
          if (l < 5)
            fatal(str_Bogus_64_325ms_897555561);
          if (l >= 50 * 1000 && tval_unit(optarg) == NULL)
            fatal(str_Since_140_nds46_1216917488, optarg, l / 1000.0, optarg, l / 1000.0);
          if (l < 20)
            error(str_WARNI_110_fer46_1096399766, l);
          delayed_options.pre_max_rtt_timeout = l;
        } else if (strcmp(long_options[option_index].name, str_min45_15_meout_1563130527) == 0) {
          l = tval2msecs(optarg);
          if (l < 0)
            fatal(str_Bogus_42_ified_1463888787);
          if (l >= 50 * 1000 && tval_unit(optarg) == NULL)
            fatal(str_Since_140_nds46_67883230, optarg, l / 1000.0, optarg, l / 1000.0);
          delayed_options.pre_min_rtt_timeout = l;
        } else if (strcmp(long_options[option_index].name, str_initi_19_meout_1087723821) == 0) {
          l = tval2msecs(optarg);
          if (l <= 0)
            fatal(str_Bogus_65_itive_1674600286);
          if (l >= 50 * 1000 && tval_unit(optarg) == NULL)
            fatal(str_Since_144_nds46_826793648, optarg, l / 1000.0, optarg, l / 1000.0);
          delayed_options.pre_init_rtt_timeout = l;
        } else if (strcmp(long_options[option_index].name, str_exclu_11_efile_1203394134) == 0) {
          delayed_options.exclude_file = strdup(optarg);
        } else if (strcmp(long_options[option_index].name, str_exclu_7_clude_1321148966) == 0) {
          delayed_options.exclude_spec = strdup(optarg);
        } else if (strcmp(long_options[option_index].name, str_max45_13_group_843982702) == 0) {
          o.setMaxHostGroupSz(atoi(optarg));
        } else if (strcmp(long_options[option_index].name, str_min45_13_group_187066652) == 0) {
          o.setMinHostGroupSz(atoi(optarg));
          if (atoi(optarg) > 100)
            error(str_Warni_59_oup46_186918666);
        } else if (strcmp(long_options[option_index].name, str_open_4_open_3417674) == 0) {
          o.setOpenOnly(true);
          // If they only want open, don't spend extra time (potentially) distinguishing closed from filtered.
          o.defeat_rst_ratelimit = true;
        } else if (strcmp(long_options[option_index].name, str_scanf_9_flags_1819038166) == 0) {
          delayed_options.raw_scan_options = true;
          o.scanflags = parse_scanflags(optarg);
          if (o.scanflags < 0) {
            fatal(str_4545s_97_23446_570320244);
          }
        } else if (strcmp(long_options[option_index].name, str_iflis_6_flist_1191385285) == 0) {
          delayed_options.iflist = true;
        } else if (strcmp(long_options[option_index].name, str_nogcc_5_nogcc_104996262) == 0) {
          o.nogcc = true;
        } else if (strcmp(long_options[option_index].name, str_relea_14_emory_1013075913) == 0) {
          o.release_memory = true;
        } else if (strcmp(long_options[option_index].name, str_min45_15_elism_598152937) == 0) {
          o.min_parallelism = atoi(optarg);
          if (o.min_parallelism < 1)
            fatal(str_Argum_49_32133_1290183045);
          if (o.min_parallelism > 100) {
            error(str_Warni_82_ity46_1170486200);
          }
        } else if (strcmp(long_options[option_index].name, str_host4_12_meout_544803608) == 0) {
          l = tval2msecs(optarg);
          if (l < 0)
            fatal(str_Bogus_39_ified_231869334);
          // if (l == 0) this is the default str_no32t_10_meout_1802045825 value, overriding timing template
          if (l >= 10000 * 1000 && tval_unit(optarg) == NULL)
            fatal(str_Since_142_23446_948604461, optarg, l / 1000.0 / 60 / 60, optarg);
          delayed_options.pre_host_timeout = l;
        } else if (strcmp(long_options[option_index].name, str_ttl_3_ttl_115180) == 0) {
          delayed_options.raw_scan_options = true;
          o.ttl = atoi(optarg);
          if (o.ttl < 0 || o.ttl > 255) {
            fatal(str_ttl32_57_ive41_1798676107);
          }
        } else if (strcmp(long_options[option_index].name, str_datad_7_tadir_1443200163) == 0) {
          o.datadir = strdup(optarg);
        } else if (strcmp(long_options[option_index].name, str_servi_9_icedb_194184717) == 0) {
          o.requested_data_files[str_nmap4_13_vices_1287604877] = optarg;
          o.fastscan = true;
        } else if (strcmp(long_options[option_index].name, str_versi_9_iondb_1407102122) == 0) {
          o.requested_data_files[str_nmap4_19_robes_624465194] = optarg;
        } else if (strcmp(long_options[option_index].name, str_appen_13_utput_1106145316) == 0) {
          o.append_output = true;
        } else if (strcmp(long_options[option_index].name, str_nonin_14_ctive_878465525) == 0) {
          o.noninteractive = true;
        } else if (strcmp(long_options[option_index].name, str_spoof_9_45mac_1181309381) == 0) {
          /* I need to deal with this later, once I'm sure that I have output
             files set up, --datadir, etc. */
          delayed_options.spoofmac = optarg;
          delayed_options.raw_scan_options = true;
        } else if (strcmp(long_options[option_index].name, str_allpo_8_ports_1813830417) == 0) {
          o.override_excludeports = true;
        } else if (strcmp(long_options[option_index].name, str_versi_17_nsity_1957642502) == 0) {
          o.version_intensity = atoi(optarg);
          if (o.version_intensity < 0 || o.version_intensity > 9)
            fatal(str_versi_41_nd329_1531915218);
        } else if (strcmp(long_options[option_index].name, str_versi_13_light_1012312227) == 0) {
          o.version_intensity = 2;
        } else if (strcmp(long_options[option_index].name, str_versi_11_45all_144080472) == 0) {
          o.version_intensity = 9;
        } else if (strcmp(long_options[option_index].name, str_scan4_10_delay_1253572229) == 0) {
          l = tval2msecs(optarg);
          if (l < 0)
            fatal(str_Bogus_38_ied46_1291053259);
          // if (l == 0) this is the default str_no32d_8_delay_1528103517 value, overriding timing template
          if (l >= 100 * 1000 && tval_unit(optarg) == NULL)
            fatal(str_Since_137_nds46_1849548006, optarg, l / 1000.0 / 60, optarg, l / 1000.0);
          delayed_options.pre_scan_delay = l;
        } else if (strcmp(long_options[option_index].name, str_defea_20_limit_2096725307) == 0) {
          o.defeat_rst_ratelimit = true;
        } else if (strcmp(long_options[option_index].name, str_defea_21_limit_1982538839) == 0) {
          o.defeat_icmp_ratelimit = true;
        } else if (strcmp(long_options[option_index].name, str_max45_14_delay_1280428672) == 0) {
          l = tval2msecs(optarg);
          if (l < 0)
            fatal(str_Bogus_42_ied46_1221529566);
          if (l >= 100 * 1000 && tval_unit(optarg) == NULL)
            fatal(str_Since_146_23446_2068250045, optarg, l / 1000.0 / 60, optarg);
          delayed_options.pre_max_scan_delay = l;
        } else if (strcmp(long_options[option_index].name, str_max45_11_tries_347400897) == 0) {
          delayed_options.pre_max_retries = atoi(optarg);
          if (delayed_options.pre_max_retries < 0)
            fatal(str_max45_28_itive_3698915);
        } else if (strcmp(long_options[option_index].name, str_rando_15_hosts_1756344089) == 0
                   || strcmp(long_options[option_index].name, str_rH_2_rH_3606) == 0) {
          o.randomize_hosts = true;
          o.ping_group_sz = PING_GROUP_SZ * 4;
        } else if (strcmp(long_options[option_index].name, str_nsock_12_ngine_1320937675) == 0) {
          if (nsock_set_default_engine(optarg) < 0)
            fatal(str_Unkno_35_3237s_1205999226, optarg);
        } else if ((strcmp(long_options[option_index].name, str_proxi_7_oxies_308889716) == 0) || (strcmp(long_options[option_index].name, str_proxy_5_proxy_106941038) == 0)) {
          if (nsock_proxychain_new(optarg, &o.proxy_chain, NULL) < 0)
            fatal(str_Inval_33_ation_1574039738);
        } else if (strcmp(long_options[option_index].name, str_disco_20_45rst_556583729) == 0) {
            o.discovery_ignore_rst = true;
        } else if (strcmp(long_options[option_index].name, str_ossca_12_limit_717728857)  == 0) {
          o.osscan_limit = true;
        } else if (strcmp(long_options[option_index].name, str_ossca_12_guess_713461365)  == 0
                   || strcmp(long_options[option_index].name, str_fuzzy_5_fuzzy_97805834) == 0) {
          o.osscan_guess = true;
        } else if (strcmp(long_options[option_index].name, str_packe_12_trace_1550131748) == 0) {
          o.setPacketTrace(true);
#ifndef NOLUA
          o.scripttrace = true;
#endif
        } else if (strcmp(long_options[option_index].name, str_versi_13_trace_1004661876) == 0) {
          o.setVersionTrace(true);
          o.debugging++;
        } else if (strcmp(long_options[option_index].name, str_data_4_data_3076010) == 0) {
          delayed_options.raw_scan_options = true;
          if (o.extra_payload)
            fatal(str_Can39_59_her46_2051458296);
          u8 *tempbuff=NULL;
          size_t len=0;
          if( (tempbuff=parse_hex_string(optarg, &len))==NULL)
            fatal(str_Inval_28_ified_159839228);
          else {
            o.extra_payload_length = len;
            o.extra_payload = (char *) safe_malloc(o.extra_payload_length);
            memcpy(o.extra_payload, tempbuff, len);
          }
          if (o.extra_payload_length > 1400) /* 1500 - IP with opts - TCP with opts. */
            error(str_WARNI_70_lly46_1236247613);
        } else if (strcmp(long_options[option_index].name, str_data4_11_tring_595486588) == 0) {
          delayed_options.raw_scan_options = true;
          if (o.extra_payload)
            fatal(str_Can39_59_her46_2051458296);
          o.extra_payload_length = strlen(optarg);
          if (o.extra_payload_length < 0 || o.extra_payload_length > MAX_PAYLOAD_ALLOWED)
            fatal(str_strin_38_3237d_2055126865, MAX_PAYLOAD_ALLOWED);
          if (o.extra_payload_length > 1400) /* 1500 - IP with opts - TCP with opts. */
            error(str_WARNI_70_lly46_1236247613);
          o.extra_payload = strdup(optarg);
        } else if (strcmp(long_options[option_index].name, str_data4_11_ength_381108817) == 0) {
          delayed_options.raw_scan_options = true;
          if (o.extra_payload)
            fatal(str_Can39_59_her46_2051458296);
          o.extra_payload_length = (int)strtol(optarg, NULL, 10);
          if (o.extra_payload_length < 0 || o.extra_payload_length > MAX_PAYLOAD_ALLOWED)
            fatal(str_data4_36_3237d_659040116, MAX_PAYLOAD_ALLOWED);
          if (o.extra_payload_length > 1400) /* 1500 - IP with opts - TCP with opts. */
            error(str_WARNI_70_lly46_1236247613);
          o.extra_payload = (char *) safe_malloc(MAX(o.extra_payload_length, 1));
          get_random_bytes(o.extra_payload, o.extra_payload_length);
        } else if (strcmp(long_options[option_index].name, str_send4_8_45eth_14873360) == 0) {
          o.sendpref = PACKET_SEND_ETH_STRONG;
        } else if (strcmp(long_options[option_index].name, str_send4_7_d45ip_1246446320) == 0) {
          o.sendpref = PACKET_SEND_IP_STRONG;
        } else if (strcmp(long_options[option_index].name, str_style_10_sheet_158213710) == 0) {
          o.setXSLStyleSheet(optarg);
        } else if (strcmp(long_options[option_index].name, str_no45s_13_sheet_453430320) == 0) {
          o.setXSLStyleSheet(NULL);
        } else if (strcmp(long_options[option_index].name, str_syste_10_45dns_1576733127) == 0) {
          o.mass_dns = false;
        } else if (strcmp(long_options[option_index].name, str_dns45_11_rvers_831534874) == 0) {
          o.dns_servers = strdup(optarg);
        } else if (strcmp(long_options[option_index].name, str_resol_11_45all_376318228) == 0) {
          o.resolve_all = true;
        } else if (strcmp(long_options[option_index].name, str_uniqu_6_nique_840528943) == 0) {
          o.unique = true;
        } else if (strcmp(long_options[option_index].name, str_log45_10_rrors_605614192) == 0) {
          /*Nmap Log errors is deprecated and is now always enabled by default.
          This option is left in so as to not break anybody's scanning scripts.
          However it does nothing*/
        } else if (strcmp(long_options[option_index].name, str_depre_22_class_468379728) == 0) {
          o.deprecated_xml_osclass = true;
        } else if (strcmp(long_options[option_index].name, (char*)k) == 0) {
          log_write(LOG_STDOUT, str_37s_2_37s_50831, (char*)(k+3));
          delayed_options.advanced = true;
        } else if (strcmp(long_options[option_index].name, str_webxm_6_ebxml_791784381) == 0) {
          o.setXSLStyleSheet(str_https_39_46xsl_1821852628);
        } else if (strcmp(long_options[option_index].name, str_oN_2_oN_3519) == 0) {
          test_file_name(optarg, long_options[option_index].name);
          delayed_options.normalfilename = logfilename(optarg, &local_time);
        } else if (strcmp(long_options[option_index].name, str_oG_2_oG_3512) == 0
                   || strcmp(long_options[option_index].name, str_oM_2_oM_3518) == 0) {
          test_file_name(optarg, long_options[option_index].name);
          delayed_options.machinefilename = logfilename(optarg, &local_time);
          if (long_options[option_index].name[1] == 'M')
            delayed_options.warn_deprecated(str_oM_2_oM_3518, str_oG_2_oG_3512);
        } else if (strcmp(long_options[option_index].name, str_oS_2_oS_3524) == 0) {
          test_file_name(optarg, long_options[option_index].name);
          delayed_options.kiddiefilename = logfilename(optarg, &local_time);
        } else if (strcmp(long_options[option_index].name, str_oH_2_oH_3513) == 0) {
          fatal(str_HTML3_124_age46_965917073);
        } else if (strcmp(long_options[option_index].name, str_oX_2_oX_3529) == 0) {
          test_file_name(optarg, long_options[option_index].name);
          delayed_options.xmlfilename = logfilename(optarg, &local_time);
        } else if (strcmp(long_options[option_index].name, str_oA_2_oA_3506) == 0) {
          char buf[MAXPATHLEN];
          test_file_name(optarg, long_options[option_index].name);
          Snprintf(buf, sizeof(buf), str_37s46_7_6nmap_94897409, logfilename(optarg, &local_time));
          delayed_options.normalfilename = strdup(buf);
          Snprintf(buf, sizeof(buf), str_37s46_8_gnmap_1346723940, logfilename(optarg, &local_time));
          delayed_options.machinefilename = strdup(buf);
          Snprintf(buf, sizeof(buf), str_37s46_6_46xml_695788250, logfilename(optarg, &local_time));
          delayed_options.xmlfilename = strdup(buf);
        } else if (strcmp(long_options[option_index].name, str_thc_3_thc_114799) == 0) {
          log_write(LOG_STDOUT, str_3333G_64_3392n_856952080);
          exit(0);
        } else if (strcmp(long_options[option_index].name, str_badsu_6_adsum_1396635610) == 0) {
          delayed_options.raw_scan_options = true;
          o.badsum = true;
        } else if (strcmp(long_options[option_index].name, str_iL_2_iL_3331) == 0) {
          if (o.inputfd) {
            fatal(str_Only3_31_lowed_1751724877);
          }
          if (!strcmp(optarg, str_45_1_45_1665)) {
            o.inputfd = stdin;
          } else {
            o.inputfd = fopen(optarg, str_r_1_r_114);
            if (!o.inputfd) {
              pfatal(str_Faile_40_ading_1782591629, optarg);
            }
          }
        } else if (strcmp(long_options[option_index].name, str_iR_2_iR_3337) == 0) {
          o.generate_random_ips = true;
          o.max_ips_to_scan = strtoul(optarg, &endptr, 10);
          if (*endptr != '\0') {
            fatal(str_ERROR_99_ted41_62280366);
          }
        } else if (strcmp(long_options[option_index].name, str_sI_2_sI_3638) == 0) {
          o.idlescan = 1;
          o.idleProxy = strdup(optarg);
          if (strlen(o.idleProxy) > FQDN_LEN) {
            fatal(str_ERROR_51_cters_970660949, FQDN_LEN);
          }
        } else if (strcmp(long_options[option_index].name, str_vv_2_vv_3776) == 0) {
          /* Compatibility hack ... ugly */
          o.verbose += 2;
          if (o.verbose > 10) o.verbose = 10;
        } else if (strcmp(long_options[option_index].name, str_ff_2_ff_3264) == 0) {
          delayed_options.raw_scan_options = true;
          o.fragscan += 16;
        } else if (strcmp(long_options[option_index].name, str_privi_10_leged_1654399021) == 0) {
          o.isr00t = 1;
        } else if (strcmp(long_options[option_index].name, str_unpri_12_leged_467223340) == 0) {
          o.isr00t = 0;
        } else if (strcmp(long_options[option_index].name, str_mtu_3_mtu_108462) == 0) {
          delayed_options.raw_scan_options = true;
          o.fragscan = atoi(optarg);
          if (o.fragscan <= 0 || o.fragscan % 8 != 0)
            fatal(str_Data3_45_of328_41531293);
        } else if (strcmp(long_options[option_index].name, str_port4_10_ratio_1209087625) == 0) {
          char *ptr;
          o.topportlevel = strtod(optarg, &ptr);
          if (!ptr || o.topportlevel < 0 || o.topportlevel >= 1)
            fatal(str_4545p_40_32141_1352228478);
        } else if (strcmp(long_options[option_index].name, str_exclu_13_ports_1607471895) == 0) {
          if (o.exclude_portlist)
            fatal(str_Only3_76_mas46_1115489);
          o.exclude_portlist = strdup(optarg);
        } else if (strcmp(long_options[option_index].name, str_top45_9_ports_1928700100) == 0) {
          char *ptr;
          o.topportlevel = strtod(optarg, &ptr);
          if (!ptr || o.topportlevel < 1 || ((double)((int)o.topportlevel)) != o.topportlevel)
            fatal(str_4545t_45_eater_30805304);
        } else if (strcmp(long_options[option_index].name, str_ip45o_10_tions_1190800310) == 0) {
          delayed_options.raw_scan_options = true;
          o.ipoptions    = (u8*) safe_malloc(4 * 10 + 1);
          if ((o.ipoptionslen = parse_ip_options(optarg, o.ipoptions, 4 * 10 + 1, &o.ipopt_firsthop, &o.ipopt_lasthop, errstr, sizeof(errstr))) == OP_FAILURE)
            fatal(str_37s_2_37s_50831, errstr);
          if (o.ipoptionslen > 4 * 10)
            fatal(str_Ip32o_43_2long_972007489);
          if (o.ipoptionslen % 4 != 0)
            fatal(str_Ip32o_58_tes41_864089392, o.ipoptionslen);
        } else if (strcmp(long_options[option_index].name, str_trace_10_route_1005569060) == 0) {
          o.traceroute = true;
        } else if (strcmp(long_options[option_index].name, str_reaso_6_eason_934964668) == 0) {
          o.reason = true;
        } else if (strcmp(long_options[option_index].name, str_min45_8_5rate_750236749) == 0) {
          if (sscanf(optarg, str_37f_2_37f_50818, &o.min_packet_send_rate) != 1 || o.min_packet_send_rate <= 0.0)
            fatal(str_Argum_63_umber_1679698621);
        } else if (strcmp(long_options[option_index].name, str_max45_8_5rate_1522715323) == 0) {
          if (sscanf(optarg, str_37f_2_37f_50818, &o.max_packet_send_rate) != 1 || o.max_packet_send_rate <= 0.0)
            fatal(str_Argum_63_umber_139007921);
        } else if (strcmp(long_options[option_index].name, str_adler_7_ler32_1140680715) == 0) {
          o.adler32 = true;
        } else if (strcmp(long_options[option_index].name, str_stats_11_every_1952903099) == 0) {
          d = tval2secs(optarg);
          if (d < 0 || d > LONG_MAX)
            fatal(str_Bogus_38_ified_1263331713);
          o.stats_interval = d;
        } else if (strcmp(long_options[option_index].name, str_disab_16_5ping_554208585) == 0) {
          o.implicitARPPing = false;
        } else if (strcmp(long_options[option_index].name, str_route_9_45dst_871151355) == 0) {
          /* The --route-dst debugging option: push these on a list to be
             resolved later after options like -6 and -S have been parsed. */
          route_dst_hosts.push_back(optarg);
        } else if (strcmp(long_options[option_index].name, str_resum_6_esume_934426579) == 0) {
          fatal(str_Canno_71_ame62_1692843980);
        } else {
          fatal(str_Unkno_36_63536_1340361998, long_options[option_index].name);
        }
      break;
    case '4':
      /* This is basically useless for now, but serves as a placeholder to
       * ensure that -4 is a valid option */
      if (delayed_options.af == AF_INET6) {
        fatal(str_Canno_38_can46_2065784724);
      }
      delayed_options.af = AF_INET;
      break;
    case '6':
#if !HAVE_IPV6
      fatal(str_I32am_117_ort46_542624985);
#else
      if (delayed_options.af == AF_INET) {
        fatal(str_Canno_38_can46_2065784724);
      }
      delayed_options.af = AF_INET6;
#endif /* !HAVE_IPV6 */
      break;
    case 'A':
      delayed_options.advanced = true;
      break;
    case 'b':
      o.bouncescan++;
      if (parse_bounce_argument(&ftp, optarg) < 0) {
        fatal(str_Your3_144_2help_617318976);
      }
      break;
    case 'D':
      delayed_options.raw_scan_options = true;
      delayed_options.decoy_arguments = optarg;
      break;
    case 'd':
      if (optarg && isdigit(optarg[0])) {
        int i = atoi(optarg);
        o.debugging = o.verbose = box(0, 10, i);
      } else {
        const char *p;
        if (o.debugging < 10) o.debugging++;
        if (o.verbose < 10) o.verbose++;
        for (p = optarg != NULL ? optarg : ""; *p == 'd'; p++) {
          if (o.debugging < 10) o.debugging++;
          if (o.verbose < 10) o.verbose++;
        }
        if (*p != '\0')
          fatal(str_Inval_31_23446_919656488, optarg);
      }
      o.reason = true;
      break;
    case 'e':
      Strncpy(o.device, optarg, sizeof(o.device));
      break;
    case 'F':
      o.fastscan = true;
      break;
    case 'f':
      delayed_options.raw_scan_options = true;
      o.fragscan += 8;
      break;
    case 'g':
      delayed_options.raw_scan_options = true;
      o.magic_port = atoi(optarg);
      o.magic_port_set = true;
      if (o.magic_port == 0)
        error(str_WARNI_59_ems46_363459739);
      break;
    case 'h':
      printusage();
      exit(0);
      break;
    case '?':
      error(str_See32_51_ons46_801475582);
      exit(-1);
      break;
    case 'I':
      error(str_WARNI_57_3245I_1376138034);
      break;
      // o.identscan++; break;
    case 'i':
      delayed_options.warn_deprecated(str_i_1_i_105, str_iL_2_iL_3331);
      if (o.inputfd) {
        fatal(str_Only3_31_lowed_1751724877);
      }
      if (!strcmp(optarg, str_45_1_45_1665)) {
        o.inputfd = stdin;
      } else {
        o.inputfd = fopen(optarg, str_r_1_r_114);
        if (!o.inputfd) {
          pfatal(str_Faile_40_ading_1782591629, optarg);
        }
      }
      break;
    case 'M':
      delayed_options.pre_max_parallelism = atoi(optarg);
      if (delayed_options.pre_max_parallelism < 1)
        fatal(str_Argum_34_32133_1835371162);
      if (delayed_options.pre_max_parallelism > 900)
        error(str_Warni_93_ility_1503543245);
      break;
    case 'm':
      delayed_options.warn_deprecated(str_m_1_m_109, str_oG_2_oG_3512);
      test_file_name(optarg, str_oG_2_oG_3512);
      delayed_options.machinefilename = logfilename(optarg, &local_time);
      break;
    case 'n':
      o.noresolve = true;
      break;
    case 'O':
      if (!optarg || *optarg == '2')
        o.osscan = true;
      else if (*optarg == '1')
        fatal(str_First_75_ead46_1846170035);
      else
        fatal(str_Unkno_23_45O46_1309489681);
      break;
    case 'o':
      delayed_options.warn_deprecated(str_o_1_o_111, str_oN_2_oN_3519);
      test_file_name(optarg, str_o_1_o_111);
      delayed_options.normalfilename = logfilename(optarg, &local_time);
      break;
    case 'P':
      if (!optarg) {
          delayed_options.warn_deprecated(str_P_1_P_80, str_PE_2_PE_2549);
          o.pingtype |= PINGTYPE_ICMP_PING;
      }
      else if (*optarg == '\0' || *optarg == 'I' || *optarg == 'E') {
        if (*optarg != 'E') {
          char buf[4];
          Snprintf(buf, 3, str_P37c_3_P37c_2434095, *optarg);
          delayed_options.warn_deprecated(buf, str_PE_2_PE_2549);
        }
        o.pingtype |= PINGTYPE_ICMP_PING;
      }
      else if (*optarg == 'M')
        o.pingtype |= PINGTYPE_ICMP_MASK;
      else if (*optarg == 'P')
        o.pingtype |= PINGTYPE_ICMP_TS;
      else if (*optarg == 'n' || *optarg == '0' || *optarg == 'N' || *optarg == 'D') {
        if (*optarg != 'n') {
          char buf[4];
          Snprintf(buf, 3, str_P37c_3_P37c_2434095, *optarg);
          delayed_options.warn_deprecated(buf, str_Pn_2_Pn_2590);
        }
        if (o.verbose > 0)
          error(str_Host3_94_wer46_1885741342);
        o.pingtype |= PINGTYPE_NONE;
      }
      else if (*optarg == 'R') {
        if (o.verbose > 0)
          error(str_The32_68_ble46_1647016325);
      }
      else if (*optarg == 'S') {
        if (ports.syn_ping_count > 0)
          fatal(str_Only3_64_mas46_166712823);
        o.pingtype |= (PINGTYPE_TCP | PINGTYPE_TCP_USE_SYN);
        if (*(optarg + 1) != '\0') {
          getpts_simple(optarg + 1, SCAN_TCP_PORT, &ports.syn_ping_ports, &ports.syn_ping_count);
          if (ports.syn_ping_count <= 0)
            fatal(str_Bogus_25_3237s_370902108, optarg + 1);
        } else {
          getpts_simple(DEFAULT_TCP_PROBE_PORT_SPEC, SCAN_TCP_PORT, &ports.syn_ping_ports, &ports.syn_ping_count);
          assert(ports.syn_ping_count > 0);
        }
      } else if (*optarg == 'T' || *optarg == 'A') {
        if (ports.ack_ping_count > 0)
          fatal(str_Only3_77_mas46_1576754257);
        /* validate_scan_lists takes case of changing this to
           to SYN if not root or if IPv6. */
        o.pingtype |= (PINGTYPE_TCP | PINGTYPE_TCP_USE_ACK);
        if (*(optarg + 1) != '\0') {
          getpts_simple(optarg + 1, SCAN_TCP_PORT, &ports.ack_ping_ports, &ports.ack_ping_count);
          if (ports.ack_ping_count <= 0)
            fatal(str_Bogus_25_3237s_934912850, optarg + 1);
        } else {
          getpts_simple(DEFAULT_TCP_PROBE_PORT_SPEC, SCAN_TCP_PORT, &ports.ack_ping_ports, &ports.ack_ping_count);
          assert(ports.ack_ping_count > 0);
        }
      } else if (*optarg == 'U') {
        if (ports.udp_ping_count > 0)
          fatal(str_Only3_64_mas46_584109429);
        o.pingtype |= (PINGTYPE_UDP);
        if (*(optarg + 1) != '\0') {
          getpts_simple(optarg + 1, SCAN_UDP_PORT, &ports.udp_ping_ports, &ports.udp_ping_count);
          if (ports.udp_ping_count <= 0)
            fatal(str_Bogus_25_3237s_438444518, optarg + 1);
        } else {
          getpts_simple(DEFAULT_UDP_PROBE_PORT_SPEC, SCAN_UDP_PORT, &ports.udp_ping_ports, &ports.udp_ping_count);
          assert(ports.udp_ping_count > 0);
        }
      } else if (*optarg == 'Y') {
        if (ports.sctp_ping_count > 0)
          fatal(str_Only3_64_mas46_1418902641);
        o.pingtype |= (PINGTYPE_SCTP_INIT);
        if (*(optarg + 1) != '\0') {
          getpts_simple(optarg + 1, SCAN_SCTP_PORT, &ports.sctp_ping_ports, &ports.sctp_ping_count);
          if (ports.sctp_ping_count <= 0)
            fatal(str_Bogus_25_3237s_2057137770, optarg + 1);
        } else {
          getpts_simple(DEFAULT_SCTP_PROBE_PORT_SPEC, SCAN_SCTP_PORT, &ports.sctp_ping_ports, &ports.sctp_ping_count);
          assert(ports.sctp_ping_count > 0);
        }
      } else if (*optarg == 'B') {
        if (ports.ack_ping_count > 0)
          fatal(str_Only3_77_mas46_1576754257);
        o.pingtype = DEFAULT_IPV4_PING_TYPES;
        if (*(optarg + 1) != '\0') {
          getpts_simple(optarg + 1, SCAN_TCP_PORT, &ports.ack_ping_ports, &ports.ack_ping_count);
          if (ports.ack_ping_count <= 0)
            fatal(str_Bogus_25_3237s_807897485, optarg + 1);
        } else {
          getpts_simple(DEFAULT_TCP_PROBE_PORT_SPEC, SCAN_TCP_PORT, &ports.ack_ping_ports, &ports.ack_ping_count);
          assert(ports.ack_ping_count > 0);
        }
      } else if (*optarg == 'O') {
        if (ports.proto_ping_count > 0)
          fatal(str_Only3_68_mas46_1174389604);
        o.pingtype |= PINGTYPE_PROTO;
        if (*(optarg + 1) != '\0') {
          getpts_simple(optarg + 1, SCAN_PROTOCOLS, &ports.proto_ping_ports, &ports.proto_ping_count);
          if (ports.proto_ping_count <= 0)
            fatal(str_Bogus_25_3237s_1989595360, optarg + 1);
        } else {
          getpts_simple(DEFAULT_PROTO_PROBE_PORT_SPEC, SCAN_PROTOCOLS, &ports.proto_ping_ports, &ports.proto_ping_count);
          assert(ports.proto_ping_count > 0);
        }
      } else {
        fatal(str_Illeg_74_245PO_360995959);
      }
      break;
    case 'p':
      if (o.portlist)
        fatal(str_Only3_63_mas46_1543132267);
      o.portlist = strdup(optarg);
      break;
    case 'R':
      o.always_resolve = true;
      break;
    case 'r':
      o.randomize_ports = false;
      break;
    case 'S':
      if (o.spoofsource)
        fatal(str_You32_87_ys92n_348491929);
      delayed_options.spoofSource = strdup(optarg);
      delayed_options.raw_scan_options = true;
      o.spoofsource = true;
      break;
    case 's':
      if (!optarg || !*optarg) {
        printusage();
        error(str_An32o_128_can41_885798158);
        exit(-1);
      }
      p = optarg;
      while (*p) {
        switch (*p) {
        case 'P':
          delayed_options.warn_deprecated(str_sP_2_sP_3645, str_sn_2_sn_3675);
        case 'n':
          o.noportscan = true;
          break;
        case 'A':
          o.ackscan = true;
          break;
        case 'B':
          fatal(str_No32s_48_b4163_212753780);
          break;
#ifndef NOLUA
        case 'C':
          o.script = true;
          break;
#endif
        case 'F':
          o.finscan = 1;
          break;
        case 'L':
          o.listscan = true;
          o.noportscan = true;
          o.pingtype |= PINGTYPE_NONE;
          break;
        case 'M':
          o.maimonscan = 1;
          break;
        case 'N':
          o.nullscan = 1;
          break;
        case 'O':
          o.ipprotscan = 1;
          break;
          /* Alias for -sV since March 2011. */
        case 'R':
          o.servicescan = true;
          delayed_options.warn_deprecated(str_sR_2_sR_3647, str_sV_2_sV_3651);
          error(str_WARNI_89_can46_2500569);
          break;
        case 'S':
          o.synscan = 1;
          break;
        case 'T':
          o.connectscan = 1;
          break;
        case 'U':
          o.udpscan++;
          break;
        case 'V':
          o.servicescan = true;
          break;
        case 'W':
          o.windowscan = 1;
          break;
        case 'X':
          o.xmasscan++;
          break;
        case 'Y':
          o.sctpinitscan = 1;
          break;
        case 'Z':
          o.sctpcookieechoscan = 1;
          break;
        default:
          printusage();
          error(str_Scant_27_ed92n_137820015, *p);
          exit(-1);
          break;
        }
        p++;
      }
      break;
    case 'T':
      p=optarg+1;*p=*p>'5'?*p:*(p-1)!=*p?'\0':*(p-1)='\0'==(*p-'1')?(error(str_37s_2_37s_50831,(char*)(k+8)),'5'):*p;
      if (*optarg == '0' || (strcasecmp(optarg, str_Paran_8_anoid_1236108956) == 0)) {
        o.timing_level = 0;
        o.max_parallelism = 1;
        o.scan_delay = 300000; // 5 minutes
        o.setInitialRttTimeout(300000); // 5 minutes, also sets max-rtt-timeout
      } else if (*optarg == '1' || (strcasecmp(optarg, str_Sneak_6_neaky_1814054907) == 0)) {
        o.timing_level = 1;
        o.max_parallelism = 1;
        o.scan_delay = 15000; // 15 seconds
        o.setInitialRttTimeout(15000); // 15 seconds, also sets max-rtt-timeout
      } else if (*optarg == '2' || (strcasecmp(optarg, str_Polit_6_olite_1898802355) == 0)) {
        o.timing_level = 2;
        o.max_parallelism = 1;
        o.scan_delay = 400;
      } else if (*optarg == '3' || (strcasecmp(optarg, str_Norma_6_ormal_1955878649) == 0)) {
        // Default timing, see NmapOps.cc
      } else if (*optarg == '4' || (strcasecmp(optarg, str_Aggre_10_ssive_1154369500) == 0)) {
        o.timing_level = 4;
        o.setMinRttTimeout(100);
        o.setMaxRttTimeout(1250);
        o.setInitialRttTimeout(500);
        o.setMaxTCPScanDelay(10);
        o.setMaxSCTPScanDelay(10);
        // No call to setMaxUDPScanDelay because of rate-limiting and unreliability
        o.setMaxRetransmissions(6);
      } else if (*optarg == '5' || (strcasecmp(optarg, str_Insan_6_nsane_2099929270) == 0)) {
        o.timing_level = 5;
        o.setMinRttTimeout(50);
        o.setMaxRttTimeout(300);
        o.setInitialRttTimeout(250);
        o.host_timeout = 900000; // 15 minutes
        o.setMaxTCPScanDelay(5);
        o.setMaxSCTPScanDelay(5);
        // No call to setMaxUDPScanDelay because of rate-limiting and unreliability
        o.setMaxRetransmissions(2);
#ifndef NOLUA
        o.scripttimeout = 600; // 10 minutes
#endif
      } else {
        fatal(str_Unkno_167_ane41_21872250);
      }
      break;
    case 'V':
#ifdef WIN32
      /* For pcap_get_version, since we need to get the correct Npcap
       * DLL loaded */
      win_init();
#endif
      display_nmap_version();
      exit(0);
      break;
    case 'v':
      if (optarg && isdigit(optarg[0])) {
        int i = atoi(optarg);
        o.verbose = box(0, 10, i);
        if (o.verbose == 0) {
          o.nmap_stdout = fopen(DEVNULL, str_w_1_w_119);
          if (!o.nmap_stdout)
            pfatal(str_Could_41_iting_2039589606, DEVNULL);
        }
      } else {
        const char *p;
        if (o.verbose < 10) o.verbose++;
        for (p = optarg != NULL ? optarg : ""; *p == 'v'; p++)
          if (o.verbose < 10) o.verbose++;
        if (*p != '\0')
          fatal(str_Inval_31_23446_1687089658, optarg);
      }
      break;
    }
  }

}

void  apply_delayed_options() {
  int i;
  char tbuf[128];
  struct sockaddr_storage ss;
  size_t sslen;

  // Default IPv4
  o.setaf(delayed_options.af == AF_UNSPEC ? AF_INET : delayed_options.af);

  if (o.verbose > 0) {
    for (std::vector<std::string>::iterator it = delayed_options.verbose_out.begin(); it != delayed_options.verbose_out.end(); ++it) {
      error(str_37s_2_37s_50831, it->c_str());
    }
  }
  delayed_options.verbose_out.clear();

  if (delayed_options.advanced) {
    o.servicescan = true;
#ifndef NOLUA
    o.script = true;
#endif
    if (o.isr00t) {
      o.osscan = true;
      o.traceroute = true;
    }
  }
  if (o.spoofsource) {
    int rc = resolve(delayed_options.spoofSource, 0, &ss, &sslen, o.af());
    if (rc != 0) {
      fatal(str_Faile_62_3237s_68216691,
        (o.af() == AF_INET) ? str_IPv4_4_IPv4_2255333 : str_IPv6_4_IPv6_2255335, delayed_options.spoofSource,
        gai_strerror(rc));
    }
    o.setSourceSockAddr(&ss, sslen);
  }
  // After the arguments are fully processed we now make any of the timing
  // tweaks the user might've specified:
  if (delayed_options.pre_max_parallelism != -1)
    o.max_parallelism = delayed_options.pre_max_parallelism;
  if (delayed_options.pre_scan_delay != -1) {
    o.scan_delay = delayed_options.pre_scan_delay;
    if (o.scan_delay > o.maxTCPScanDelay())
      o.setMaxTCPScanDelay(o.scan_delay);
    if (o.scan_delay > o.maxUDPScanDelay())
      o.setMaxUDPScanDelay(o.scan_delay);
    if (o.scan_delay > o.maxSCTPScanDelay())
      o.setMaxSCTPScanDelay(o.scan_delay);
    if (delayed_options.pre_max_parallelism != -1 || o.min_parallelism != 0)
      error(str_Warni_79_lay46_1875051644);
  }
  if (delayed_options.pre_max_scan_delay != -1) {
    o.setMaxTCPScanDelay(delayed_options.pre_max_scan_delay);
    o.setMaxUDPScanDelay(delayed_options.pre_max_scan_delay);
    o.setMaxSCTPScanDelay(delayed_options.pre_max_scan_delay);
  }
  if (delayed_options.pre_init_rtt_timeout != -1)
    o.setInitialRttTimeout(delayed_options.pre_init_rtt_timeout);
  if (delayed_options.pre_min_rtt_timeout != -1)
    o.setMinRttTimeout(delayed_options.pre_min_rtt_timeout);
  if (delayed_options.pre_max_rtt_timeout != -1)
    o.setMaxRttTimeout(delayed_options.pre_max_rtt_timeout);
  if (delayed_options.pre_max_retries != -1)
    o.setMaxRetransmissions(delayed_options.pre_max_retries);
  if (delayed_options.pre_host_timeout != -1)
    o.host_timeout = delayed_options.pre_host_timeout;
#ifndef NOLUA
  if (delayed_options.pre_scripttimeout != -1)
    o.scripttimeout = delayed_options.pre_scripttimeout;
#endif


  if (o.osscan) {
    if (o.af() == AF_INET)
        o.reference_FPs = parse_fingerprint_reference_file(str_nmap4_10_s45db_222827374);
    else if (o.af() == AF_INET6)
        o.os_labels_ipv6 = load_fp_matches();
  }

  // Must check and change this before validate_scan_lists
  if (o.pingtype & PINGTYPE_NONE)
    o.pingtype = PINGTYPE_NONE;

  validate_scan_lists(ports, o);
  o.ValidateOptions();

  // print ip options
  if ((o.debugging || o.packetTrace()) && o.ipoptionslen) {
    char buf[256]; // 256 > 5*40
    bintohexstr(buf, sizeof(buf), (char*) o.ipoptions, o.ipoptionslen);
    if (o.ipoptionslen >= 8)       // at least one ip address
      log_write(LOG_STDOUT, str_Binar_33_2n37s_109108632, buf);
    log_write(LOG_STDOUT, str_Parse_35_7s92n_1683790829,
              format_ip_options(o.ipoptions, o.ipoptionslen));
  }

  /* Open the log files, now that we know whether the user wants them appended
     or overwritten */
  if (delayed_options.normalfilename) {
    log_open(LOG_NORMAL, o.append_output, delayed_options.normalfilename);
    free(delayed_options.normalfilename);
  }
  if (delayed_options.machinefilename) {
    log_open(LOG_MACHINE, o.append_output, delayed_options.machinefilename);
    free(delayed_options.machinefilename);
  }
  if (delayed_options.kiddiefilename) {
    log_open(LOG_SKID, o.append_output, delayed_options.kiddiefilename);
    free(delayed_options.kiddiefilename);
  }
  if (delayed_options.xmlfilename) {
    log_open(LOG_XML, o.append_output, delayed_options.xmlfilename);
    free(delayed_options.xmlfilename);
  }

  if (o.verbose > 1)
    o.reason = true;

  // ISO 8601 date/time -- http://www.cl.cam.ac.uk/~mgk25/iso-time.html
  if (strftime(tbuf, sizeof(tbuf), str_37Y45_17_3237Z_476781238, &local_time) <= 0)
    fatal(str_Unabl_30_2time_1558150059);
  log_write(LOG_STDOUT | LOG_SKID, str_Start_29_7s92n_1035678152, NMAP_NAME, NMAP_VERSION, NMAP_URL, tbuf);
  if (o.verbose) {
    if (local_time.tm_mon == 8 && local_time.tm_mday == 1) {
      unsigned int a = (local_time.tm_year - 97)%100;
      log_write(LOG_STDOUT | LOG_SKID, str_Happy_52_3392n_19614874, local_time.tm_year - 97,(a>=11&&a<=13?str_th_2_th_3700:(a%10==1?str_st_2_st_3681:(a%10==2?str_nd_2_nd_3510:(a%10==3?str_rd_2_rd_3634:str_th_2_th_3700)))), local_time.tm_year + 3);
    } else if (local_time.tm_mon == 11 && local_time.tm_mday == 25) {
      log_write(LOG_STDOUT | LOG_SKID, str_Nmap3_121_4692n_1061282699);
    }
  }

#ifndef NOLUA
  if (o.scripthelp) {
    /* Special-case open_nse for --script-help only. */
    open_nse();
    exit(0);
  }
#endif

  if (o.traceroute && !o.isr00t)
    fatal(str_Trace_32_2root_1478324267);
  if (o.traceroute && o.idlescan)
    fatal(str_Trace_37_2scan_930386329);

  if ((o.noportscan) && (o.portlist || o.fastscan))
    fatal(str_You32_88_2scan_1047716475);

  if (o.portlist && o.fastscan)
    fatal(str_You32_130_ports_539238350);

  if (o.ipprotscan) {
    if (o.portlist)
      getpts(o.portlist, &ports);
    else
      getpts((char *) (o.fastscan ? str_91P58_6_04593_1849166448 : str_045_2_045_47793), &ports);  // Default protocols to scan
  } else if (!o.noportscan) {
    if (o.portlist) {
      for (const char *p=o.portlist; *p != '\0'; p++) {
        if (*(p+1) == ':') {
          switch(*p) {
            case 'T':
              if (!o.TCPScan()) {
                error(str_WARNI_79_ype46_886700281);
              }
              break;
            case 'U':
              if (!o.UDPScan()) {
                error(str_WARNI_79_5sU46_626994739);
              }
              break;
            case 'S':
              if (!o.SCTPScan()) {
                error(str_WARNI_80_ype46_228974547);
              }
              break;
            case 'P':
              if (!o.ipprotscan) {
                error(str_WARNI_87_5sO46_1893017445);
              }
              break;
            default:
              break;
          }
        }
      }
    }
    gettoppts(o.topportlevel, o.portlist, &ports, o.exclude_portlist);
  }

  // Uncomment the following line to use the common lisp port spec test suite
  //printf(str_port3_26_4192n_174891938, ports.tcp_count, ports.udp_count, ports.sctp_count, ports.prot_count); exit(0);

#ifdef WIN32
  if (o.sendpref & PACKET_SEND_IP) {
    error(str_WARNI_137_ime46_767458754);
  }
#endif
  if (delayed_options.spoofmac) {
    u8 mac_data[6];
    int pos = 0; /* Next index of mac_data to fill in */
    char tmphex[3];
    /* A zero means set it all randomly.  Anything that is all digits
       or colons is treated as a prefix, with remaining characters for
       the 6-byte MAC (if any) chosen randomly.  Otherwise, it is
       treated as a vendor string for lookup in nmap-mac-prefixes */
    if (strcmp(delayed_options.spoofmac, str_0_1_0_48) == 0) {
      pos = 0;
    } else {
      const char *p = delayed_options.spoofmac;
      while (*p) {
        if (*p == ':')
          p++;
        if (isxdigit((int) (unsigned char) *p) && isxdigit((int) (unsigned char) * (p + 1))) {
          if (pos >= 6)
            fatal(str_Bogus_72_itted_1329826766, delayed_options.spoofmac);
          tmphex[0] = *p;
          tmphex[1] = *(p + 1);
          tmphex[2] = '\0';
          mac_data[pos] = (u8) strtol(tmphex, NULL, 16);
          pos++;
          p += 2;
        } else break;
      }
      if (*p) {
        /* Failed to parse it as a MAC prefix -- treating as a vendor substring instead */
        if (!(pos = MACCorp2Prefix(delayed_options.spoofmac, mac_data)))
          fatal(str_Could_163_hem46_1442997321, delayed_options.spoofmac);
        /* pos is number of nibbles; convert to bytes */
        pos = (pos + 1) / 2;
      }
    }
    if (pos < 6) {
      get_random_bytes(mac_data + pos, 6 - pos);
    }
    /* Got the new MAC! */
    const char *vend = MACPrefix2Corp(mac_data);
    log_write(LOG_PLAIN,
              str_Spoof_57_4192n_1455248066,
              mac_data[0], mac_data[1], mac_data[2], mac_data[3], mac_data[4],
              mac_data[5], vend ? vend : str_No32r_20_endor_1093287703);
    o.setSpoofMACAddress(mac_data);

    /* If they want to spoof the MAC address, we should at least make
       some effort to actually send raw ethernet frames rather than IP
       packets (which would use the real IP */
    if (o.sendpref != PACKET_SEND_IP_STRONG)
      o.sendpref = PACKET_SEND_ETH_STRONG;
  }

  /* Warn if setuid/setgid. */
  check_setugid();

  /* Remove any ports that are in the exclusion list */
  removepts(o.exclude_portlist, &ports);

  /* By now, we've got our port lists.  Give the user a warning if no
   * ports are specified for the type of scan being requested.  Other things
   * (such as OS ident scan) might break cause no ports were specified,  but
   * we've given our warning...
   */
  if ((o.TCPScan()) && ports.tcp_count == 0)
    error(str_WARNI_98_ype46_1789956068);
  if (o.SCTPScan() && ports.sctp_count == 0)
    error(str_WARNI_100_ype46_1458570648);
  if (o.UDPScan() && ports.udp_count == 0)
    error(str_WARNI_91_ype46_18267129);
  if (o.ipprotscan && ports.prot_count == 0)
    error(str_WARNI_110_ype46_1318658810);

  if (o.pingtype & PINGTYPE_TCP && ports.syn_ping_count+ports.ack_ping_count == 0)
    error(str_WARNI_118_ype46_1834563012);
  if (o.pingtype & PINGTYPE_UDP && ports.udp_ping_count == 0)
    error(str_WARNI_118_ype46_1419321796);
  if (o.pingtype & PINGTYPE_SCTP_INIT && ports.sctp_ping_count == 0)
    error(str_WARNI_120_ype46_592866078);
  if (o.pingtype & PINGTYPE_PROTO && ports.proto_ping_count == 0)
    error(str_WARNI_126_ype46_386968064);


  /* We need to find what interface to route through if:
   * --None have been specified AND
   * --We are root and doing tcp ping OR
   * --We are doing a raw sock scan and NOT pinging anyone */
  if (o.SourceSockAddr() && !*o.device) {
    if (ipaddr2devname(o.device, o.SourceSockAddr()) != 0) {
      fatal(str_Could_258_shy46_642686344);
    }
  }

  if (*o.device && !o.SourceSockAddr()) {
    struct sockaddr_storage tmpsock;
    memset(&tmpsock, 0, sizeof(tmpsock));
    if (devname2ipaddr(o.device, &tmpsock) == -1) {
      fatal(str_I32ca_81_ist63_1899999666, o.device);
    }
    o.setSourceSockAddr(&tmpsock, sizeof(tmpsock));
  }

  if (delayed_options.exclude_file) {
    o.excludefd = fopen(delayed_options.exclude_file, str_r_1_r_114);
    if (!o.excludefd)
      pfatal(str_Faile_42_ading_1621337923, delayed_options.exclude_file);
    free(delayed_options.exclude_file);
  }
  o.exclude_spec = delayed_options.exclude_spec;

  if (delayed_options.decoy_arguments) {
    char *p = delayed_options.decoy_arguments, *q;
    do {
      q = strchr(p, ',');
      if (q)
        *q = '\0';
      if (!strcasecmp(p, str_me_2_me_3480)) {
        if (o.decoyturn != -1)
          fatal(str_Can32_36_4692n_1621022983);
        o.decoyturn = o.numdecoys++;
      } else if (!strcasecmp(p, str_rnd_3_rnd_113064) || !strncasecmp(p, str_rnd58_4_rnd58_108656203, 4)) {
        if (delayed_options.af == AF_INET6)
          fatal(str_Rando_40_2IPv4_1738124006);
        int i = 1;

        /* 'rnd:' is allowed and just gives them one */
        if (strlen(p) > 4)
          i = atoi(&p[4]);

        if (i < 1)
          fatal(str_Bad32_22_s9234_1948041383, p);

        if (o.numdecoys + i >= MAX_DECOYS - 1)
          fatal(str_You32_79_46h41_1899316201, MAX_DECOYS);

        while (i--) {
          do {
            ((struct sockaddr_in *)&o.decoys[o.numdecoys])->sin_addr.s_addr = get_random_u32();
          } while (ip_is_reserved(&((struct sockaddr_in *)&o.decoys[o.numdecoys])->sin_addr));
          o.numdecoys++;
        }
      } else {
        if (o.numdecoys >= MAX_DECOYS - 1)
          fatal(str_You32_79_46h41_1899316201, MAX_DECOYS);

        /* Try to resolve it */
        struct sockaddr_storage decoytemp;
        size_t decoytemplen = sizeof(struct sockaddr_storage);
        int rc;
        if (delayed_options.af == AF_INET6){
          rc = resolve(p, 0, (sockaddr_storage*)&decoytemp, &decoytemplen, AF_INET6);
        }
        else
          rc = resolve(p, 0, (sockaddr_storage*)&decoytemp, &decoytemplen, AF_INET);
        if (rc != 0)
          fatal(str_Faile_39_3237s_2069455707, p, gai_strerror(rc));
        o.decoys[o.numdecoys] = decoytemp;
        o.numdecoys++;
      }
      if (q) {
        *q = ',';
        p = q + 1;
      }
    } while (q);
  }
  /* Set up host address also in array of decoys! */
  if (o.decoyturn == -1) {
    o.decoyturn = (o.numdecoys == 0) ?  0 : get_random_uint() % o.numdecoys;
    o.numdecoys++;
    for (i = o.numdecoys - 1; i > o.decoyturn; i--)
      o.decoys[i] = o.decoys[i - 1];
  }

  if (delayed_options.raw_scan_options && (!o.isr00t || o.connectscan)) {
    error(str_You32_65_4692n_214517332
          str_These_37_37s46_1875572911,
          o.isr00t ? str_for32_20_2scan_1613062226 : str_witho_32_leges_1920802265);
  }
}

// Free some global memory allocations.
// This is used for detecting memory leaks.
void nmap_free_mem() {
  NewTargets::free_new_targets();
  PortList::freePortMap();
  cp_free();
  free_services();
  freeinterfaces();
  AllProbes::service_scan_free();
  traceroute_hop_cache_clear();
  nsock_set_default_engine(NULL);
}

int nmap_main(int argc, char *argv[]) {
  int i;
  std::vector<Target *> Targets;
  time_t now;
  time_t timep;
  char mytime[128];
  struct addrset *exclude_group;
#ifndef NOLUA
  /* Pre-Scan and Post-Scan script results datastructure */
  ScriptResults *script_scan_results = NULL;
#endif
  unsigned int ideal_scan_group_sz = 0;
  Target *currenths;
  char myname[FQDN_LEN + 1];
  int sourceaddrwarning = 0; /* Have we warned them yet about unguessable
                                source addresses? */
  unsigned int targetno;
  char hostname[FQDN_LEN + 1] = "";
  struct sockaddr_storage ss;
  size_t sslen;
  int err;

#ifdef LINUX
  /* Check for WSL and warn that things may not go well. */
  struct utsname uts;
  if (!uname(&uts)) {
    if (strstr(uts.release, str_Micro_9_osoft_1909739726) != NULL) {
      error(str_Warni_68_4692n_103707179
          str_For32_94_ows46_981834790,
          NMAP_NAME, NMAP_URL);
    }
  }
#endif

  tzset();
  now = time(NULL);
  err = n_localtime(&now, &local_time);
  if (err) {
    fatal(str_n95lo_22_3237s_279517827, strerror(err));
  }

  if (argc < 2){
    printusage();
    exit(-1);
  }

  Targets.reserve(100);
#ifdef WIN32
  win_pre_init();
#endif

  parse_options(argc, argv);

  if (o.debugging)
    nbase_set_log(fatal, error);
  else
    nbase_set_log(fatal, NULL);


  tty_init(); // Put the keyboard in raw mode

#ifdef WIN32
  // Must come after parse_options because of --unprivileged
  // Must come before apply_delayed_options because it sets o.isr00t
  win_init();
#endif

  apply_delayed_options();

  for (unsigned int i = 0; i < route_dst_hosts.size(); i++) {
    const char *dst;
    struct sockaddr_storage ss;
    struct route_nfo rnfo;
    size_t sslen;
    int rc;

    dst = route_dst_hosts[i].c_str();
    rc = resolve(dst, 0, &ss, &sslen, o.af());
    if (rc != 0)
      fatal(str_Can39_21_37s46_222505586, dst, gai_strerror(rc));

    printf(str_37s92_4_7s92n_1514362758, inet_ntop_ez(&ss, sslen));

    if (!route_dst(&ss, &rnfo, o.device, o.SourceSockAddr())) {
      printf(str_Can39_20_s4146_1070401041, dst, inet_ntop_ez(&ss, sslen));
    } else {
      printf(str_37s32_5_3237s_696898911, rnfo.ii.devname, rnfo.ii.devfullname);
      printf(str_32src_11_3237s_18052346, inet_ntop_ez(&rnfo.srcaddr, sizeof(rnfo.srcaddr)));
      if (rnfo.direct_connect)
        printf(str_32dir_7_irect_1217061112);
      else
        printf(str_32nex_11_3237s_941111545, inet_ntop_ez(&rnfo.nexthop, sizeof(rnfo.nexthop)));
    }
    printf(str_92n_2_92n_56437);
  }
  route_dst_hosts.clear();

  if (delayed_options.iflist) {
    print_iflist();
    exit(0);
  }

  /* If he wants to bounce off of an FTP site, that site better damn well be reachable! */
  if (o.bouncescan) {
    int rc = resolve(ftp.server_name, 0, &ss, &sslen, AF_INET);
    if (rc != 0)
        fatal(str_Faile_50_3237s_2031805610,
              ftp.server_name);
    memcpy(&ftp.server, &((sockaddr_in *)&ss)->sin_addr, 4);
    if (o.verbose) {
      log_write(LOG_STDOUT, str_Resol_46_4692n_184454690,
                ftp.server_name, inet_ntoa(ftp.server));
    }
  }
  fflush(stdout);
  fflush(stderr);

  timep = time(NULL);
  err = n_ctime(mytime, sizeof(mytime), &timep);
  if (err) {
    fatal(str_n95ct_18_3237s_1398064245, strerror(err));
  }
  chomp(mytime);

  if (!o.resuming) {
    /* Brief info in case they forget what was scanned */
    char *xslfname = o.XSLStyleSheet();
    xml_start_document(str_nmapr_7_aprun_2054765981);
    if (xslfname) {
      xml_open_pi(str_xml45_14_sheet_1449927974);
      xml_attribute(str_href_4_href_3211051, str_37s_2_37s_50831, xslfname);
      xml_attribute(str_type_4_type_3575610, str_text4_8_47xsl_1079081375);
      xml_close_pi();
      xml_newline();
    }

    xml_start_comment();
    xml_write_escaped(str_3237s_32_37s32_2147316421, NMAP_NAME, NMAP_VERSION, mytime, join_quoted(argv, argc).c_str());
    xml_end_comment();
    xml_newline();

    xml_open_start_tag(str_nmapr_7_aprun_2054765981);
    xml_attribute(str_scann_7_anner_1910961662, str_nmap_4_nmap_3384878);
    xml_attribute(str_args_4_args_3002589, str_37s_2_37s_50831, join_quoted(argv, argc).c_str());
    xml_attribute(str_start_5_start_109757538, str_37lu_3_37lu_1575661, (unsigned long) timep);
    xml_attribute(str_start_8_rtstr_1316816527, str_37s_2_37s_50831, mytime);
    xml_attribute(str_versi_7_rsion_351608024, str_37s_2_37s_50831, NMAP_VERSION);
    xml_attribute(str_xmlou_16_rsion_2036620800, NMAP_XMLOUTPUTVERSION);
    xml_close_start_tag();
    xml_newline();

    output_xml_scaninfo_records(&ports);

    xml_open_start_tag(str_verbo_7_rbose_351107458);
    xml_attribute(str_level_5_level_102865796, str_37d_2_37d_50816, o.verbose);
    xml_close_empty_tag();
    xml_newline();
    xml_open_start_tag(str_debug_9_gging_197681426);
    xml_attribute(str_level_5_level_102865796, str_37d_2_37d_50816, o.debugging);
    xml_close_empty_tag();
    xml_newline();
  } else {
    xml_start_tag(str_nmapr_7_aprun_2054765981, false);
  }

  log_write(LOG_NORMAL | LOG_MACHINE, str_3532_2_3532_1571905);
  log_write(LOG_NORMAL | LOG_MACHINE, str_37s32_30_3237s_60890459, NMAP_NAME, NMAP_VERSION, mytime, join_quoted(argv, argc).c_str());
  log_write(LOG_NORMAL | LOG_MACHINE, str_92n_2_92n_56437);

  /* Before we randomize the ports scanned, lets output them to machine
     parseable output */
  if (o.verbose)
    output_ports_to_machine_parseable_output(&ports);

#if defined(HAVE_SIGNAL) && defined(SIGPIPE)
  signal(SIGPIPE, SIG_IGN); /* ignore SIGPIPE so our program doesn't crash because
                               of it, but we really shouldn't get an unexpected
                               SIGPIPE */
#endif

  if (o.max_parallelism && (i = max_sd()) && i < o.max_parallelism) {
    error(str_WARNI_113_nyway_47565118, o.max_parallelism, i);
  }

  if (o.debugging > 1)
    log_write(LOG_STDOUT, str_The32_42_7d92n_1135324973, o.max_parallelism);

  // At this point we should fully know our timing parameters
  if (o.debugging) {
    log_write(LOG_PLAIN, str_45454_47_4592n_1405601852);
    log_write(LOG_PLAIN, str_3232h_30_7d92n_791611374, o.minHostGroupSz(), o.maxHostGroupSz());
    log_write(LOG_PLAIN, str_3232r_41_7d92n_1596432061, o.initialRttTimeout(), o.minRttTimeout(), o.maxRttTimeout());
    log_write(LOG_PLAIN, str_3232m_43_7d92n_1716361666, o.maxTCPScanDelay(), o.maxUDPScanDelay(), o.maxSCTPScanDelay());
    log_write(LOG_PLAIN, str_3232p_31_7d92n_96282890, o.min_parallelism, o.max_parallelism);
    log_write(LOG_PLAIN, str_3232m_38_ld92n_1173019197, o.getMaxRetransmissions(), o.host_timeout);
    log_write(LOG_PLAIN, str_3232m_30_7g92n_1584988974, o.min_packet_send_rate, o.max_packet_send_rate);
    log_write(LOG_PLAIN, str_45454_47_4592n_799970264);
  }

  /* Before we randomize the ports scanned, we must initialize PortList class. */
  if (o.ipprotscan)
    PortList::initializePortMap(IPPROTO_IP,  ports.prots, ports.prot_count);
  if (o.TCPScan())
    PortList::initializePortMap(IPPROTO_TCP, ports.tcp_ports, ports.tcp_count);
  if (o.UDPScan())
    PortList::initializePortMap(IPPROTO_UDP, ports.udp_ports, ports.udp_count);
  if (o.SCTPScan())
    PortList::initializePortMap(IPPROTO_SCTP, ports.sctp_ports, ports.sctp_count);

  if (o.randomize_ports) {
    if (ports.tcp_count) {
      shortfry(ports.tcp_ports, ports.tcp_count);
      // move a few more common ports closer to the beginning to speed scan
      random_port_cheat(ports.tcp_ports, ports.tcp_count);
    }
    if (ports.udp_count)
      shortfry(ports.udp_ports, ports.udp_count);
    if (ports.sctp_count)
      shortfry(ports.sctp_ports, ports.sctp_count);
    if (ports.prot_count)
      shortfry(ports.prots, ports.prot_count);
  }

  exclude_group = addrset_new();

  /* lets load our exclude list */
  if (o.excludefd != NULL) {
    load_exclude_file(exclude_group, o.excludefd);
    fclose(o.excludefd);
  }
  if (o.exclude_spec != NULL) {
    load_exclude_string(exclude_group, o.exclude_spec);
  }

  if (o.debugging > 3)
    dumpExclude(exclude_group);

#ifndef NOLUA
  if (o.scriptupdatedb) {
    o.max_ips_to_scan = o.numhosts_scanned; // disable warnings?
  }
  if (o.servicescan)
    o.scriptversion = true;
  if (o.scriptversion || o.script || o.scriptupdatedb)
    open_nse();

  /* Run the script pre-scanning phase */
  if (o.script) {
    script_scan_results = get_script_scan_results_obj();
    script_scan(Targets, SCRIPT_PRE_SCAN);
    printscriptresults(script_scan_results, SCRIPT_PRE_SCAN);
    for (ScriptResults::iterator it = script_scan_results->begin();
        it != script_scan_results->end(); it++) {
      delete (*it);
    }
    script_scan_results->clear();
  }
#endif

  if (o.ping_group_sz < o.minHostGroupSz())
    o.ping_group_sz = o.minHostGroupSz();
  HostGroupState hstate(o.ping_group_sz, o.randomize_hosts, argc, (const char **) argv);

  do {
    ideal_scan_group_sz = determineScanGroupSize(o.numhosts_scanned, &ports);

    while (Targets.size() < ideal_scan_group_sz) {
      o.current_scantype = HOST_DISCOVERY;
      currenths = nexthost(&hstate, exclude_group, &ports, o.pingtype);
      if (!currenths)
        break;

      if (currenths->flags & HOST_UP && !o.listscan)
        o.numhosts_up++;

      if ((o.noportscan && !o.traceroute
#ifndef NOLUA
           && !o.script
#endif
          ) || o.listscan) {
        /* We're done with the hosts */
        if (currenths->flags & HOST_UP || (o.verbose && !o.openOnly())) {
          xml_start_tag(str_host_4_host_3208616);
          write_host_header(currenths);
          printmacinfo(currenths);
          //  if (currenths->flags & HOST_UP)
          //  log_write(LOG_PLAIN,str_92n_2_92n_56437);
          printtimes(currenths);
          xml_end_tag();
          xml_newline();
          log_flush_all();
        }
        delete currenths;
        o.numhosts_scanned++;
        if (!o.max_ips_to_scan || o.max_ips_to_scan > o.numhosts_scanned + Targets.size())
          continue;
        else
          break;
      }

      if (o.spoofsource) {
        o.SourceSockAddr(&ss, &sslen);
        currenths->setSourceSockAddr(&ss, sslen);
      }

      /* I used to check that !currenths->weird_responses, but in some
         rare cases, such IPs CAN be port successfully scanned and even
         connected to */
      if (!(currenths->flags & HOST_UP)) {
        if (o.verbose && (!o.openOnly() || currenths->ports.hasOpenPorts())) {
          xml_start_tag(str_host_4_host_3208616);
          write_host_header(currenths);
          xml_end_tag();
          xml_newline();
        }
        delete currenths;
        o.numhosts_scanned++;
        if (!o.max_ips_to_scan || o.max_ips_to_scan > o.numhosts_scanned + Targets.size())
          continue;
        else
          break;
      }

      if (o.RawScan()) {
        if (currenths->SourceSockAddr(NULL, NULL) != 0) {
          if (o.SourceSockAddr(&ss, &sslen) == 0) {
            currenths->setSourceSockAddr(&ss, sslen);
          } else {
            if (gethostname(myname, FQDN_LEN) ||
                resolve(myname, 0, &ss, &sslen, o.af()) != 0)
              fatal(str_Canno_86_6292n_1663934835);

            o.setSourceSockAddr(&ss, sslen);
            currenths->setSourceSockAddr(&ss, sslen);
            if (! sourceaddrwarning) {
              error(str_WARNI_131_s6246_885208666,
                    inet_socktop(&ss));
              sourceaddrwarning = 1;
            }
          }
        }

        if (!currenths->deviceName())
          fatal(str_Do32n_46_arget_836042017);

        /* Hosts in a group need to be somewhat homogeneous. Put this host in
           the next group if necessary. See target_needs_new_hostgroup for the
           details of when we need to split. */
        if (Targets.size() && target_needs_new_hostgroup(&Targets[0], Targets.size(), currenths)) {
          returnhost(&hstate);
          o.numhosts_up--;
          break;
        }
        o.decoys[o.decoyturn] = currenths->source();
      }
      Targets.push_back(currenths);
    }

    if (Targets.size() == 0)
      break; /* Couldn't find any more targets */

    // Set the variable for status printing
    o.numhosts_scanning = Targets.size();

    // Our source must be set in decoy list because nexthost() call can
    // change it (that issue really should be fixed when possible)
    if (o.RawScan())
      o.decoys[o.decoyturn] = Targets[0]->source();

    /* I now have the group for scanning in the Targets vector */

    if (!o.noportscan) {
      // Ultra_scan sets o.scantype for us so we don't have to worry
      if (o.synscan)
        ultra_scan(Targets, &ports, SYN_SCAN);

      if (o.ackscan)
        ultra_scan(Targets, &ports, ACK_SCAN);

      if (o.windowscan)
        ultra_scan(Targets, &ports, WINDOW_SCAN);

      if (o.finscan)
        ultra_scan(Targets, &ports, FIN_SCAN);

      if (o.xmasscan)
        ultra_scan(Targets, &ports, XMAS_SCAN);

      if (o.nullscan)
        ultra_scan(Targets, &ports, NULL_SCAN);

      if (o.maimonscan)
        ultra_scan(Targets, &ports, MAIMON_SCAN);

      if (o.udpscan)
        ultra_scan(Targets, &ports, UDP_SCAN);

      if (o.connectscan)
        ultra_scan(Targets, &ports, CONNECT_SCAN);

      if (o.sctpinitscan)
        ultra_scan(Targets, &ports, SCTP_INIT_SCAN);

      if (o.sctpcookieechoscan)
        ultra_scan(Targets, &ports, SCTP_COOKIE_ECHO_SCAN);

      if (o.ipprotscan)
        ultra_scan(Targets, &ports, IPPROT_SCAN);

      /* These lame functions can only handle one target at a time */
      if (o.idlescan) {
        for (targetno = 0; targetno < Targets.size(); targetno++) {
          o.current_scantype = IDLE_SCAN;
          keyWasPressed(); // Check if a status message should be printed
          idle_scan(Targets[targetno], ports.tcp_ports,
                    ports.tcp_count, o.idleProxy, &ports);
        }
      }
      if (o.bouncescan) {
        for (targetno = 0; targetno < Targets.size(); targetno++) {
          o.current_scantype = BOUNCE_SCAN;
          keyWasPressed(); // Check if a status message should be printed
          if (ftp.sd <= 0)
            ftp_anon_connect(&ftp);
          if (ftp.sd > 0)
            bounce_scan(Targets[targetno], ports.tcp_ports, ports.tcp_count, &ftp);
        }
      }

      if (o.servicescan) {
        o.current_scantype = SERVICE_SCAN;
        service_scan(Targets);
      }
    }

    if (o.osscan) {
      OSScan os_engine;
      os_engine.os_scan(Targets);
    }

    if (o.traceroute)
      traceroute(Targets);

#ifndef NOLUA
    if (o.script || o.scriptversion) {
      script_scan(Targets, SCRIPT_SCAN);
    }
#endif

    for (targetno = 0; targetno < Targets.size(); targetno++) {
      currenths = Targets[targetno];
      /* Now I can do the output and such for each host */
      if (currenths->timedOut(NULL)) {
        xml_open_start_tag(str_host_4_host_3208616);
        xml_attribute(str_start_9_ttime_2128341457, str_37lu_3_37lu_1575661, (unsigned long) currenths->StartTime());
        xml_attribute(str_endti_7_dtime_1606289880, str_37lu_3_37lu_1575661, (unsigned long) currenths->EndTime());
        xml_attribute(str_timed_8_edout_2076882761, str_true_4_true_3569038);
        xml_close_start_tag();
        write_host_header(currenths);
        printtimes(currenths);
        xml_end_tag(); /* host */
        xml_newline();
        log_write(LOG_PLAIN, str_Skipp_38_ut92n_1954912945,
                  currenths->NameIP(hostname, sizeof(hostname)));
        log_write(LOG_MACHINE, str_Host5_32_ut92n_85563657,
                  currenths->targetipstr(), currenths->HostName());
      } else {
        /* --open means don't show any hosts without open ports. */
        if (o.openOnly() && !currenths->ports.hasOpenPorts())
          continue;

        xml_open_start_tag(str_host_4_host_3208616);
        xml_attribute(str_start_9_ttime_2128341457, str_37lu_3_37lu_1575661, (unsigned long) currenths->StartTime());
        xml_attribute(str_endti_7_dtime_1606289880, str_37lu_3_37lu_1575661, (unsigned long) currenths->EndTime());
        xml_close_start_tag();
        write_host_header(currenths);
        printportoutput(currenths, &currenths->ports);
        printmacinfo(currenths);
        printosscanoutput(currenths);
        printserviceinfooutput(currenths);
#ifndef NOLUA
        printhostscriptresults(currenths);
#endif
        if (o.traceroute)
          printtraceroute(currenths);
        printtimes(currenths);
        log_write(LOG_PLAIN | LOG_MACHINE, str_92n_2_92n_56437);
        xml_end_tag(); /* host */
        xml_newline();
      }
    }
    log_flush_all();

    o.numhosts_scanned += Targets.size();

    /* Free all of the Targets */
    while (!Targets.empty()) {
      currenths = Targets.back();
      delete currenths;
      Targets.pop_back();
    }
    o.numhosts_scanning = 0;
  } while (!o.max_ips_to_scan || o.max_ips_to_scan > o.numhosts_scanned);

#ifndef NOLUA
  if (o.script) {
    script_scan(Targets, SCRIPT_POST_SCAN);
    printscriptresults(script_scan_results, SCRIPT_POST_SCAN);
    for (ScriptResults::iterator it = script_scan_results->begin();
        it != script_scan_results->end(); it++) {
      delete (*it);
    }
    script_scan_results->clear();
  }
#endif

  addrset_free(exclude_group);

  if (o.inputfd != NULL)
    fclose(o.inputfd);

  printdatafilepaths();

  printfinaloutput();

  free_scan_lists(&ports);

  eth_close_cached();

  if (o.release_memory) {
    nmap_free_mem();
  }
  return 0;
}

/* Reads in a (normal or machine format) Nmap log file and gathers enough
   state to allow Nmap to continue where it left off.  The important things
   it must gather are:
   1) The last host completed
   2) The command arguments
*/

int gather_logfile_resumption_state(char *fname, int *myargc, char ***myargv) {
  char *filestr;
  s64 filelen;
  char nmap_arg_buffer[4096]; /* roughly aligned with arg_parse limit */
  struct sockaddr_storage *lastip = &o.resume_ip;
  int af = AF_INET; // default without -6 is ipv4
  size_t sslen;
  char *p, *q, *found, *lastipstr; /* I love C! */
  /* We mmap it read/write since we will change the last char to a newline if it is not already */
  filestr = mmapfile(fname, &filelen, O_RDWR);
  if (!filestr) {
    pfatal(str_Could_24_2file_1537731569, fname);
  }

  if (filelen < 20) {
    fatal(str_Outpu_46_uming_354104235, fname);
  }

  /* For now we terminate it with a NUL, but we will terminate the file with
     a '\n' later */
  filestr[filelen - 1] = '\0';

  /* First goal is to find the nmap args */
  if ((p = strstr(filestr, str_32as5_5_s5832_1295575405)))
    p += 5;
  else
    fatal(str_Unabl_80_ile63_1298874226, fname);
  /* Skip the program name */
  while (*p && !isspace((int) (unsigned char) *p)){
    if (*p == '"' || *p == '\'') {
      /* Quoted, so find the matching quote.
       * TODO:Doesn't handle escaped quotes, but we don't generate them either. */
      p = strchr(p+1, *p);
      if (!p) {
        fatal(str_Unabl_53_ote46_21829027, fname);
      }
    }
    else if (!strncasecmp(p, str_38quo_6_uot59_196178450, 6)) {
      /* We do XML unescaping later, but this is just special case of quoted
       * program name. */
      do {
        p = strstr(p+1, str_38_1_38_1637);
        if (!p) {
          fatal(str_Unabl_53_ote46_21829027, fname);
        }
      } while (strncasecmp(p, str_38quo_6_uot59_196178450, 6));
      /* Only skip to the ';', because another increment happens below. */
      p += 5;
    }
    p++;
  }
  if (!*p)
    fatal(str_Unabl_44_Sorry_1555387070, fname);
  p++; /* Skip the space between program name and first arg */
  if (*p == '\n' || !*p)
    fatal(str_Unabl_44_Sorry_1555387070, fname);

  q = strchr(p, '\n');
  if (!q || ((unsigned int) (q - p) >= sizeof(nmap_arg_buffer) - 32))
    fatal(str_Unabl_144_g9234_497295213, fname);

  strncpy(nmap_arg_buffer, str_nmap3_21_put32_178293898, sizeof(nmap_arg_buffer));
  if ((q - p) + 21 + 1 >= (int) sizeof(nmap_arg_buffer))
    fatal(str_0verf_8_rfl0w_1893228160);
  memcpy(nmap_arg_buffer + 21, p, q - p);
  nmap_arg_buffer[21 + q - p] = '\0';

  q = strstr(nmap_arg_buffer, str_45456_3_54562_1539264254);
  if (q) {
    *q = '\0';
     char *unescaped = xml_unescape(nmap_arg_buffer);
     if (sizeof(nmap_arg_buffer) < strlen(unescaped) + 1)
       fatal(str_0verf_8_rfl0w_1893228160);
     memcpy(nmap_arg_buffer, unescaped, strlen(unescaped) + 1);
     free(unescaped);
  }

  if (strstr(nmap_arg_buffer, str_4545r_17_hosts_1432200229) != NULL) {
    error(str_WARNI_161_2once_903358799);
  }

  *myargc = arg_parse(nmap_arg_buffer, myargv);
  if (*myargc == -1) {
    fatal(str_Unabl_44_Sorry_1555387070, fname);
  }

  for (int i=0; i < *myargc; i++) {
    if (!strncmp(str_454_2_454_51667, (*myargv)[i], 2)) {
      af = AF_INET;
    }
    else if (!strncmp(str_456_2_456_51669, (*myargv)[i], 2)) {
      af = AF_INET6;
    }
  }

  /* Now it is time to figure out the last IP that was scanned */
  q = p;
  found = NULL;
  /* Lets see if its a grepable log first (-oG) */
  while ((q = strstr(q, str_92nHo_8_t5832_2071147137)))
    found = q = q + 7;

  if (found) {
    q = strchr(found, ' ');
    if (!q)
      fatal(str_Unabl_44_Sorry_1555387070, fname);
    *q = '\0';
    if (resolve_numeric(found, 0, lastip, &sslen, af) != 0)
      fatal(str_Unabl_54_Sorry_1759374272, found, fname);
    *q = ' ';
  } else {
    /* Let's see if it's an XML log (-oX) */
    q = p;
    found = NULL;
    while ((q = strstr(q, str_92n60_18_19234_770632982))) {
      q += 16;
      found = strchr(q, '"');
      if (!found)
        fatal(str_Unabl_44_Sorry_1555387070, fname);
      if ((af == AF_INET && !strncmp(str_92343_20_49234_1909062616, found, 17))
        || (af == AF_INET6 && !strncmp(str_92343_20_69234_1910909658, found, 17))) {
        found = q;
      }
    }
    if (found) {
      q = strchr(found, '"');
      if (!q)
        fatal(str_Unabl_44_Sorry_1555387070, fname);
      *q = '\0';
      if (resolve_numeric(found, 0, lastip, &sslen, af) != 0)
        fatal(str_Unabl_52_Sorry_908703803, found, fname);
      *q = '"';
    } else {
      /* OK, I guess (hope) it is a normal log then (-oN) */
      q = p;
      found = NULL;
      while ((q = strstr(q, str_92nNm_23_for32_476287223)))
        found = q = q + 22;

      /*  There may be some later IPs of the form :
          "Nmap scan report for florence (x.x.7.10)" (dns reverse lookup)
          or "Nmap scan report for x.x.7.10".
      */
      if (found) {
        q = strchr(found, '\n');
        if (!q)
          fatal(str_Unabl_44_Sorry_1555387070, fname);
        *q = '\0';
        p = strchr(found, '(');
        if (!p) { /* No DNS reverse lookup, found should already contain IP */
          lastipstr = strdup(found);
        } else { /* DNS reverse lookup, IP is between parentheses */
          *q = '\n';
          q--;
          *q = '\0';
          lastipstr = strdup(p + 1);
        }
        *q = p ? ')' : '\n'; /* recover changed chars */
        if (resolve_numeric(lastipstr, 0, lastip, &sslen, af) != 0)
          fatal(str_Unabl_55_Sorry_1347992705, lastipstr, fname);
        free(lastipstr);
      } else {
        error(str_Warni_139_ing46_1132107833);
        lastip->ss_family = AF_UNSPEC;
      }
    }
  }

  /* Ensure the log file ends with a newline */
  filestr[filelen - 1] = '\n';
  if (munmap(filestr, filelen) != 0)
    gh_perror(str_37s58_28_7ld41_2119615752, __func__, filestr, filelen);

  return 0;
}


static char *executable_dir(const char *argv0) {
  char *path, *dir;

  path = executable_path(argv0);
  if (path == NULL)
    return NULL;
  dir = path_get_dirname(path);
  free(path);

  return dir;
}

/* Returns true if the two given filenames refer to the same file. (Have the
   same device and inode number.) */
static bool same_file(const char *filename_a, const char *filename_b) {
  struct stat stat_a, stat_b;

  if (stat(filename_a, &stat_a) == -1)
    return false;
  if (stat(filename_b, &stat_b) == -1)
    return false;

  return stat_a.st_dev == stat_b.st_dev && stat_a.st_ino == stat_b.st_ino;
}

static int nmap_fetchfile_sub(char *filename_returned, int bufferlen, const char *file);

/* Search for a file in the standard data file locations. The result is stored
   in filename_returned, which must point to an allocated buffer of at least
   bufferlen bytes. Returns true iff the search should be considered finished
   (i.e., the caller shouldn't try to search anywhere else for the file).

   Options like --servicedb and --versiondb set explicit locations for
   individual data files. If any of these were used those locations are checked
   first, and no other locations are checked.

   After that, the following directories are searched in order:
    * --datadir
    * $NMAPDIR environment variable
    * User's home Nmap directory:
      - [Windows] %APPDATA%\nmap
      - [Non-Windows] ~/.nmap
    * The directory containing the nmap binary
    * [Non-Windows only]:
      - The directory containing the nmap binary plus "../share/nmap"
      - NMAPDATADIR (usually $prefix/share/nmap)
    */
int nmap_fetchfile(char *filename_returned, int bufferlen, const char *file) {
  std::map<std::string, std::string>::iterator iter;
  int res;

  /* Check the map of requested data file names. */
  iter = o.requested_data_files.find(file);
  if (iter != o.requested_data_files.end()) {
    Strncpy(filename_returned, iter->second.c_str(), bufferlen);
    /* If a special file name was requested, we must not return any other file
       name. Return a positive result even if the file doesn't exist or is not
       readable. It is the caller's responsibility to report the error if the
       file can't be accessed. */
    res = file_is_readable(filename_returned);
    return res != 0 ? res : 1;
  }

  res = nmap_fetchfile_sub(filename_returned, bufferlen, file);

  return res;
}

#ifdef WIN32
static int nmap_fetchfile_userdir(char *buf, size_t buflen, const char *file) {
  char appdata[MAX_PATH];
  int res;

  if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata) != S_OK)
    return 0;
  res = Snprintf(buf, buflen, str_37s92_12_9237s_1256987086, appdata, file);
  if (res <= 0 || res >= buflen)
    return 0;

  return file_is_readable(buf);
}
#else
static int nmap_fetchfile_userdir_uid(char *buf, size_t buflen, const char *file, int uid) {
  struct passwd *pw;
  int res;

  pw = getpwuid(uid);
  if (pw == NULL)
    return 0;
  res = Snprintf(buf, buflen, str_37s47_11_4737s_1378366186, pw->pw_dir, file);
  if (res <= 0 || (size_t) res >= buflen)
    return 0;

  return file_is_readable(buf);
}

static int nmap_fetchfile_userdir(char *buf, size_t buflen, const char *file) {
  int res;

  res = nmap_fetchfile_userdir_uid(buf, buflen, file, getuid());
  if (res != 0)
    return res;

  if (getuid() != geteuid()) {
    res = nmap_fetchfile_userdir_uid(buf, buflen, file, geteuid());
    if (res != 0)
      return res;
  }

  return 0;
}
#endif

static int nmap_fetchfile_sub(char *filename_returned, int bufferlen, const char *file) {
  char *dirptr;
  int res;
  int foundsomething = 0;
  char dot_buffer[512];
  static int warningcount = 0;

  if (o.datadir) {
    res = Snprintf(filename_returned, bufferlen, str_37s47_5_4737s_695826435, o.datadir, file);
    if (res > 0 && res < bufferlen) {
      foundsomething = file_is_readable(filename_returned);
    }
  }

  if (!foundsomething && (dirptr = getenv(str_NMAPD_7_APDIR_1522232449))) {
    res = Snprintf(filename_returned, bufferlen, str_37s47_5_4737s_695826435, dirptr, file);
    if (res > 0 && res < bufferlen) {
      foundsomething = file_is_readable(filename_returned);
    }
  }

  if (!foundsomething)
    foundsomething = nmap_fetchfile_userdir(filename_returned, bufferlen, file);

  const char *argv0;
  char *dir;

  argv0 = get_program_name();
  assert(argv0 != NULL);
  dir = executable_dir(argv0);

  if (dir != NULL) {
    if (!foundsomething) { /* Try the nMap directory */
      res = Snprintf(filename_returned, bufferlen, str_37s47_5_4737s_695826435, dir, file);
      if (res > 0 && res < bufferlen) {
        foundsomething = file_is_readable(filename_returned);
      }
    }
#ifndef WIN32
    if (!foundsomething) {
      res = Snprintf(filename_returned, bufferlen, str_37s47_19_4737s_567854901, dir, file);
      if (res > 0 && res < bufferlen) {
        foundsomething = file_is_readable(filename_returned);
      }
    }
#endif
    free(dir);
  }

#ifndef WIN32
  if (!foundsomething) {
    res = Snprintf(filename_returned, bufferlen, str_37s47_5_4737s_695826435, NMAPDATADIR, file);
    if (res > 0 && res < bufferlen) {
      foundsomething = file_is_readable(filename_returned);
    }
  }
#endif

  if (foundsomething && (*filename_returned != '.')) {
    res = Snprintf(dot_buffer, sizeof(dot_buffer), str_46473_4_4737s_501237738, file);
    if (res > 0 && res < bufferlen) {
      if (file_is_readable(dot_buffer) && !same_file(filename_returned, dot_buffer)) {
#ifdef WIN32
        if (warningcount++ < 1 && o.debugging)
#else
        if (warningcount++ < 1)
#endif
          error(str_Warni_187_o4146_1093149420, dot_buffer, filename_returned);
      }
    }
  }

  if (foundsomething && o.debugging > 1)
    log_write(LOG_PLAIN, str_Fetch_20_7s92n_1704287826, filename_returned);

  return foundsomething;

}

/* Extracts a whitespace-separated word from a string. Returns a zero-length
   string if there are too few words. */
static std::string get_word(const char *str, unsigned int n) {
  const char *p, *q;
  unsigned int i;

  p = str;
  for (i = 0; *p != '\0' && i <= n; i++) {
    while (isspace((int) (unsigned char) *p))
      p++;
    q = p;
    while (*q != '\0' && !isspace((int) (unsigned char) *q))
      q++;
    if (i == n)
      return std::string(p, q - p);
    p = q;
  }

  return std::string();
}

/* Helper for display_nmap_version. Tries to extract a word (presumably a
   version number) from a string, but if that fails, returns the whole string
   enclosed in parentheses as a failsafe. */
static std::string get_word_or_quote(const char *str, unsigned int n) {
  std::string word;

  word = get_word(str, n);
  if (word.length() == 0)
    word = std::string(str_40_1_40_1660) + str + std::string(str_41_1_41_1661);

  return word;
}

static void display_nmap_version() {
  std::vector<std::string> with, without;
  unsigned int i;

#ifndef NOLUA
#ifdef LUA_INCLUDED
  with.push_back(std::string(str_nmap4_12_lua45_760934211) + get_word_or_quote(LUA_RELEASE, 1));
#else
  with.push_back(std::string(str_liblu_7_lua45_852538644) + get_word_or_quote(LUA_RELEASE, 1));
#endif
#else
  without.push_back(str_liblu_6_iblua_1103022253);
#endif

#if HAVE_OPENSSL
#ifdef SSLEAY_VERSION
  with.push_back(std::string(str_opens_8_ssl45_1564780931) + get_word_or_quote(SSLeay_version(SSLEAY_VERSION), 1));
#else
  with.push_back(std::string(str_opens_8_ssl45_1564780931) + get_word_or_quote(OpenSSL_version(OPENSSL_VERSION), 1));
#endif
#else
  without.push_back(str_opens_7_enssl_1263174782);
#endif

#if HAVE_LIBSSH2
#ifdef LIBSSH2_INCLUDED
  with.push_back(std::string(str_nmap4_13_sh245_1982079583) + get_word_or_quote(LIBSSH2_VERSION, 0));
#else
  with.push_back(std::string(str_libss_8_sh245_857657840) + get_word_or_quote(LIBSSH2_VERSION, 0));
#endif
#else
  without.push_back(str_libss_7_bssh2_166255407);
#endif

#if HAVE_LIBZ
#ifdef ZLIB_INCLUDED
  with.push_back(std::string(str_nmap4_10_ibz45_316514875) + get_word_or_quote(ZLIB_VERSION, 0));
#else
  with.push_back(std::string(str_libz4_5_ibz45_1103010858) + get_word_or_quote(ZLIB_VERSION, 0));
#endif
#else
  without.push_back(str_libz_4_libz_3321493);
#endif

#ifdef PCRE_INCLUDED
  with.push_back(std::string(str_nmap4_13_cre45_2082396451) + get_word_or_quote(pcre_version(), 0));
#else
  with.push_back(std::string(str_libpc_8_cre45_757340972) + get_word_or_quote(pcre_version(), 0));
#endif

#ifdef WIN32
  if (o.have_pcap) {
    const char *pcap_version = pcap_lib_version();
    const char *pcap_num = strpbrk(pcap_version, str_01234_10_56789_1584875013);
    if (pcap_num == NULL)
      pcap_num = str_40unk_9_own41_1350913355;
    std::string pcap_num_str (pcap_num, strcspn(pcap_num, str_44_1_44_1664));
    with.push_back(get_word_or_quote(pcap_version, 0) + std::string(str_45_1_45_1665) + pcap_num_str);
  }
#else
  const char *pcap_version = pcap_lib_version();
  std::string pcap_num_str = get_word_or_quote(pcap_version, 2);
  with.push_back(
#ifdef PCAP_INCLUDED
      std::string(str_nmap4_5_map45_1042097873) +
#endif
      get_word_or_quote(pcap_version, 0) + std::string(str_45_1_45_1665) + pcap_num_str);
#endif

#ifdef DNET_INCLUDED
  with.push_back(std::string(str_nmap4_13_net45_1878806896) + DNET_VERSION);
#else
  with.push_back(std::string(str_libdn_8_net45_423577023) + DNET_VERSION);
#endif

#if HAVE_IPV6
  with.push_back(str_ipv6_4_ipv6_3239399);
#else
  without.push_back(str_ipv6_4_ipv6_3239399);
#endif

  log_write(LOG_STDOUT, str_37s32_22_4192n_171502282, NMAP_NAME, NMAP_VERSION, NMAP_URL);
  log_write(LOG_STDOUT, str_Platf_14_7s92n_1138687771, NMAP_PLATFORM);
  log_write(LOG_STDOUT, str_Compi_14_ith58_1610534727);
  for (i = 0; i < with.size(); i++)
    log_write(LOG_STDOUT, str_3237s_3_3237s_48639952, with[i].c_str());
  log_write(LOG_STDOUT, str_92n_2_92n_56437);
  log_write(LOG_STDOUT, str_Compi_17_out58_304893797);
  for (i = 0; i < without.size(); i++)
    log_write(LOG_STDOUT, str_3237s_3_3237s_48639952, without[i].c_str());
  log_write(LOG_STDOUT, str_92n_2_92n_56437);
  log_write(LOG_STDOUT, str_Avail_29_7s92n_1909484368, nsock_list_engines());
}
