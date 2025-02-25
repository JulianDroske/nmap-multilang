#include "./multilang/zh_CN/output.h"

/***************************************************************************
 * output.cc -- Handles the Nmap output system.  This currently involves   *
 * console-style human readable output, XML output, Script |<iddi3         *
 * output, and the legacy grepable output (used to be called "machine      *
 * readable").  I expect that future output forms (such as HTML) may be    *
 * created by a different program, library, or script using the XML        *
 * output.                                                                 *
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

#include "nmap.h"
#include "output.h"
#include "osscan.h"
#include "osscan2.h"
#include "NmapOps.h"
#include "NmapOutputTable.h"
#include "MACLookup.h"
#include "portreasons.h"
#include "protocols.h"
#include "FingerPrintResults.h"
#include "tcpip.h"
#include "Target.h"
#include "nmap_error.h"
#include "utils.h"
#include "xml.h"
#include "nbase.h"
#include "libnetutil/netutil.h"
#include <nsock.h>

#include <math.h>

#include <set>
#include <vector>
#include <list>
#include <sstream>

extern NmapOps o;
static const char *logtypes[LOG_NUM_FILES] = LOG_NAMES;

/* Used in creating skript kiddie style output.  |<-R4d! */
static void skid_output(char *s) {
  int i;
  for (i = 0; s[i]; i++)
    /* We need a 50/50 chance here, use a random number */
    if ((get_random_u8() & 0x01) == 0)
      /* Substitutions commented out are not known to me, but maybe look nice */
      switch (s[i]) {
      case 'A':
        s[i] = '4';
        break;
        /*    case 'B': s[i]='8'; break;
           case 'b': s[i]='6'; break;
           case 'c': s[i]='k'; break;
           case 'C': s[i]='K'; break; */
      case 'e':
      case 'E':
        s[i] = '3';
        break;
      case 'i':
      case 'I':
        s[i] = str_33124_3_31241_1508695742[get_random_u8() % 3];
        break;
        /*      case 'k': s[i]='c'; break;
           case 'K': s[i]='C'; break; */
      case 'o':
      case 'O':
        s[i] = '0';
        break;
      case 's':
      case 'S':
        if (s[i + 1] && !isalnum((int) (unsigned char) s[i + 1]))
          s[i] = 'z';
        else
          s[i] = '$';
        break;
      case 'z':
        s[i] = 's';
        break;
      case 'Z':
        s[i] = 'S';
        break;
    } else {
      if (s[i] >= 'A' && s[i] <= 'Z' && (get_random_u8() % 3 == 0)) {
        s[i] += 'a' - 'A';      /* 1/3 chance of lower-case */
      } else if (s[i] >= 'a' && s[i] <= 'z' && (get_random_u8() % 3 == 0)) {
        s[i] -= 'a' - 'A';      /* 1/3 chance of upper-case */
      }
    }
}

/* Remove all str_92nSF_5_nSF58_583688747 from fingerprints */
static char *servicefp_sf_remove(const char *str) {
  char *temp = (char *) safe_malloc(strlen(str) + 1);
  char *dst = temp, *src = (char *) str;
  char *ampptr = 0;

  while (*src) {
    if (strncmp(src, "\nSF:", 4) == 0) {
      src += 4;
      continue;
    }
    /* Needed so "&something;" is not truncated midway */
    if (*src == '&') {
      ampptr = dst;
    } else if (*src == ';') {
      ampptr = 0;
    }
    *dst++ = *src++;
  }
  if (ampptr != 0) {
    *ampptr = '\0';
  } else {
    *dst = '\0';
  }
  return temp;
}

// Prints an XML <service> element for the information given in
// serviceDeduction.  This function should only be called if ether
// the service name or the service fingerprint is non-null.
static void print_xml_service(const struct serviceDeductions *sd) {
  xml_open_start_tag(str_servi_7_rvice_1984153269);

  xml_attribute(str_name_4_name_3373707, str_37s_2_37s_50831, sd->name ? sd->name : str_unkno_7_known_284840886);
  if (sd->product)
    xml_attribute(str_produ_7_oduct_309474065, str_37s_2_37s_50831, sd->product);
  if (sd->version)
    xml_attribute(str_versi_7_rsion_351608024, str_37s_2_37s_50831, sd->version);
  if (sd->extrainfo)
    xml_attribute(str_extra_9_ainfo_252677954, str_37s_2_37s_50831, sd->extrainfo);
  if (sd->hostname)
    xml_attribute(str_hostn_8_tname_299803597, str_37s_2_37s_50831, sd->hostname);
  if (sd->ostype)
    xml_attribute(str_ostyp_6_stype_1007351010, str_37s_2_37s_50831, sd->ostype);
  if (sd->devicetype)
    xml_attribute(str_devic_10_etype_782144144, str_37s_2_37s_50831, sd->devicetype);
  if (sd->service_fp) {
    char *servicefp = servicefp_sf_remove(sd->service_fp);
    xml_attribute(str_servi_9_icefp_194184641, str_37s_2_37s_50831, servicefp);
    free(servicefp);
  }

  if (sd->service_tunnel == SERVICE_TUNNEL_SSL)
    xml_attribute(str_tunne_6_unnel_862547864, str_ssl_3_ssl_114188);
  xml_attribute(str_metho_6_ethod_1077554975, str_37s_2_37s_50831, (sd->dtype == SERVICE_DETECTION_TABLE) ? str_table_5_table_110115790 : str_probe_6_robed_979816780);
  xml_attribute(str_conf_4_conf_3059492, str_37i_2_37i_50821, sd->name_confidence);

  if (sd->cpe.empty()) {
    xml_close_empty_tag();
  } else {
    unsigned int i;

    xml_close_start_tag();
    for (i = 0; i < sd->cpe.size(); i++) {
      xml_start_tag(str_cpe_3_cpe_98712);
      xml_write_escaped(str_37s_2_37s_50831, sd->cpe[i]);
      xml_end_tag();
    }
    xml_end_tag();
  }
}

#ifdef WIN32
/* Show a fatal error explaining that an interface is not Ethernet and won't
   work on Windows. Do nothing if --send-ip (PACKET_SEND_IP_STRONG) was used. */
void win32_fatal_raw_sockets(const char *devname) {
  if ((o.sendpref & PACKET_SEND_IP_STRONG) != 0)
    return;

  if (devname != NULL) {
    fatal(str_Only3_65_nd92n_1088054302
          str_92343_65_on92n_562137941
          str_for32_14_can46_559930980, devname);
  } else {
    fatal(str_Only3_65_se92n_1429210256
          str_the32_40_can46_1486694509);
  }
}

/* Display the mapping from libdnet interface names (like "eth0") to Npcap
   interface names (like "\Device\NPF_{...}"). This is the same mapping used by
   eth_open and so can help diagnose connection problems.  Additionally display
   Npcap interface names that are not mapped to by any libdnet name, in other
   words the names of interfaces Nmap has no way of using.*/
static void print_iflist_pcap_mapping(const struct interface_info *iflist,
                                      int numifs) {
  pcap_if_t *pcap_ifs = NULL;
  char errbuf[PCAP_ERRBUF_SIZE];
  std::list<const pcap_if_t *> leftover_pcap_ifs;
  std::list<const pcap_if_t *>::iterator leftover_p;
  int i;

  /* Build a list of "leftover" libpcap interfaces. Initially it contains all
     the interfaces. */
  if (o.have_pcap) {
    if (pcap_findalldevs(&pcap_ifs, errbuf) == -1) {
      fatal(str_pcap9_55_3237s_325620210, errbuf);
    }
    for (const pcap_if_t *p = pcap_ifs; p != NULL; p = p->next)
      leftover_pcap_ifs.push_front(p);
  }

  if (numifs > 0 || !leftover_pcap_ifs.empty()) {
    NmapOutputTable Tbl(1 + numifs + leftover_pcap_ifs.size(), 2);

    Tbl.addItem(0, 0, false, str_DEV_3_DEV_67573);
    Tbl.addItem(0, 1, false, str_WINDE_9_EVICE_1734630126);

    /* Show the libdnet names and what they map to. */
    for (i = 0; i < numifs; i++) {
      char pcap_name[1024];

      if (DnetName2PcapName(iflist[i].devname, pcap_name, sizeof(pcap_name))) {
        /* We got a name. Remove it from the list of leftovers. */
        std::list<const pcap_if_t *>::iterator next;
        for (leftover_p = leftover_pcap_ifs.begin();
             leftover_p != leftover_pcap_ifs.end(); leftover_p = next) {
          next = leftover_p;
          next++;
          if (strcmp((*leftover_p)->name, pcap_name) == 0)
            leftover_pcap_ifs.erase(leftover_p);
        }
      } else {
        Strncpy(pcap_name, str_60non_6_one62_1766892754, sizeof(pcap_name));
      }

      Tbl.addItem(i + 1, 0, false, iflist[i].devname);
      Tbl.addItem(i + 1, 1, true, pcap_name);
    }

    /* Show the "leftover" libpcap interface names (those without a libdnet
       name that maps to them). */
    for (leftover_p = leftover_pcap_ifs.begin();
         leftover_p != leftover_pcap_ifs.end();
         leftover_p++) {
      Tbl.addItem(i + 1, 0, false, str_60non_6_one62_1766892754);
      Tbl.addItem(i + 1, 1, false, (*leftover_p)->name);
      i++;
    }

    log_write(LOG_PLAIN, str_37s92_4_7s92n_1514362758, Tbl.printableTable(NULL));
    log_flush_all();
  }

  if (pcap_ifs) {
    pcap_freealldevs(pcap_ifs);
  }
}
#endif

/* Print a detailed list of Nmap interfaces and routes to
   normal/skiddy/stdout output */
int print_iflist(void) {
  int numifs = 0, numroutes = 0;
  struct interface_info *iflist;
  struct sys_route *routes;
  NmapOutputTable *Tbl = NULL;
  char errstr[256];
  const char *address = NULL;
  errstr[0]='\0';

  iflist = getinterfaces(&numifs, errstr, sizeof(errstr));

  int i;
  /* First let's handle interfaces ... */
  if (iflist==NULL || numifs<=0) {
    log_write(LOG_PLAIN, str_INTER_27_4192n_1745026483);
    if (o.debugging)
      log_write(LOG_STDOUT, str_Reaso_12_7s92n_566124460, errstr);
  } else {
    int devcol = 0, shortdevcol = 1, ipcol = 2, typecol = 3, upcol = 4, mtucol = 5, maccol = 6;
    Tbl = new NmapOutputTable(numifs + 1, 7);
    Tbl->addItem(0, devcol, false, str_DEV_3_DEV_67573, 3);
    Tbl->addItem(0, shortdevcol, false, str_40SHO_7_ORT41_1042342365, 7);
    Tbl->addItem(0, ipcol, false, str_IP47M_7_7MASK_2096145450, 7);
    Tbl->addItem(0, typecol, false, str_TYPE_4_TYPE_2590522, 4);
    Tbl->addItem(0, upcol, false, str_UP_2_UP_2715, 2);
    Tbl->addItem(0, mtucol, false, str_MTU_3_MTU_76686, 3);
    Tbl->addItem(0, maccol, false, str_MAC_3_MAC_76079, 3);
    for (i = 0; i < numifs; i++) {
      Tbl->addItem(i + 1, devcol, false, iflist[i].devfullname);
      Tbl->addItemFormatted(i + 1, shortdevcol, false, str_4037s_4_37s41_328600656,
                            iflist[i].devname);
      address = inet_ntop_ez(&(iflist[i].addr), sizeof(iflist[i].addr));
      Tbl->addItemFormatted(i + 1, ipcol, false, str_37s47_5_4737d_695826450,
                            address == NULL ? str_40non_6_one41_957546191 : address,
                            iflist[i].netmask_bits);
      if (iflist[i].device_type == devt_ethernet) {
        Tbl->addItem(i + 1, typecol, false, str_ether_8_ernet_1419358249);
        Tbl->addItemFormatted(i + 1, maccol, false,
                              str_3702X_29_3702X_877469763,
                              iflist[i].mac[0], iflist[i].mac[1],
                              iflist[i].mac[2], iflist[i].mac[3],
                              iflist[i].mac[4], iflist[i].mac[5]);
      } else if (iflist[i].device_type == devt_loopback)
        Tbl->addItem(i + 1, typecol, false, str_loopb_8_pback_2037065333);
      else if (iflist[i].device_type == devt_p2p)
        Tbl->addItem(i + 1, typecol, false, str_point_11_point_692285906);
      else
        Tbl->addItem(i + 1, typecol, false, str_other_5_other_106069776);
      Tbl->addItem(i + 1, upcol, false,
                   (iflist[i].device_up ? str_up_2_up_3739 : str_down_4_down_3089570));
      Tbl->addItemFormatted(i + 1, mtucol, false, str_37d_2_37d_50816, iflist[i].mtu);
    }
    log_write(LOG_PLAIN, str_42424_60_4292n_212438981);
    log_write(LOG_PLAIN, str_37s92_4_7s92n_1514362758, Tbl->printableTable(NULL));
    log_flush_all();
    delete Tbl;
  }

#ifdef WIN32
  /* Print the libdnet->libpcap interface name mapping. */
  print_iflist_pcap_mapping(iflist, numifs);
#endif

  /* OK -- time to handle routes */
  errstr[0]='\0';
  routes = getsysroutes(&numroutes, errstr, sizeof(errstr));
  u16 nbits;
  if (routes==NULL || numroutes<= 0) {
    log_write(LOG_PLAIN, str_ROUTE_23_4192n_1843030211);
    if (o.debugging)
      log_write(LOG_STDOUT, str_Reaso_12_7s92n_566124460, errstr);
  } else {
    int dstcol = 0, devcol = 1, metcol = 2, gwcol = 3;
    Tbl = new NmapOutputTable(numroutes + 1, 4);
    Tbl->addItem(0, dstcol, false, str_DST47_8_7MASK_945715756, 8);
    Tbl->addItem(0, devcol, false, str_DEV_3_DEV_67573, 3);
    Tbl->addItem(0, metcol, false, str_METRI_6_ETRIC_2024216144, 6);
    Tbl->addItem(0, gwcol, false, str_GATEW_7_TEWAY_528863780, 7);
    for (i = 0; i < numroutes; i++) {
      nbits = routes[i].netmask_bits;
      Tbl->addItemFormatted(i + 1, dstcol, false, str_37s47_5_4737d_695826450,
      	inet_ntop_ez(&routes[i].dest, sizeof(routes[i].dest)), nbits);
      Tbl->addItem(i + 1, devcol, false, routes[i].device->devfullname);
      Tbl->addItemFormatted(i + 1, metcol, false, str_37d_2_37d_50816, routes[i].metric);
      if (!sockaddr_equal_zero(&routes[i].gw))
        Tbl->addItem(i + 1, gwcol, true, inet_ntop_ez(&routes[i].gw, sizeof(routes[i].gw)));
    }
    log_write(LOG_PLAIN, str_42424_60_4292n_1267364013);
    log_write(LOG_PLAIN, str_37s92_4_7s92n_1514362758, Tbl->printableTable(NULL));
    log_flush_all();
    delete Tbl;
  }
  return 0;
}

#ifndef NOLUA
/* Escape control characters to make a string safe to display on a terminal. */
static std::string escape_for_screen(const std::string s) {
  std::string r;

  for (unsigned int i = 0; i < s.size(); i++) {
    char buf[5];
    unsigned char c = s[i];
    // Printable and some whitespace ok. str_92r_2_92r_56441 not ok because it overwrites the line.
    if (c == '\t' || c == '\n' || (0x20 <= c && c <= 0x7e)) {
      r += c;
    } else {
      Snprintf(buf, sizeof(buf), str_9292x_7_3702X_1618976084, c);
      r += buf;
    }
  }

  return r;
}

/* Do something to protect characters that can't appear in XML. This is not a
   reversible transform, more a last-ditch effort to write readable XML with
   characters that shouldn't be part of regular output anyway. The escaping that
   xml_write_escaped is not enough; some characters are not allowed to appear in
   XML, not even escaped. */
std::string protect_xml(const std::string s) {
  std::string r;

  for (unsigned int i = 0; i < s.size(); i++) {
    char buf[5];
    unsigned char c = s[i];
    // Printable and some whitespace ok.
    if (c == '\t' || c == '\r' || c == '\n' || (0x20 <= c && c <= 0x7e)) {
      r += c;
    } else {
      Snprintf(buf, sizeof(buf), str_9292x_7_3702X_1618976084, c);
      r += buf;
    }
  }

  return r;
}

static char *formatScriptOutput(const ScriptResult *sr) {
  std::vector<std::string> lines;

  std::string c_output;
  const char *p, *q;
  std::string result;
  unsigned int i;

  c_output = escape_for_screen(sr->get_output_str());
  if (c_output.empty())
    return NULL;
  p = c_output.c_str();

  while (*p != '\0') {
    q = strchr(p, '\n');
    if (q == NULL) {
      lines.push_back(std::string(p));
      break;
    } else {
      lines.push_back(std::string(p, q - p));
      p = q + 1;
    }
  }

  if (lines.empty())
    lines.push_back("");
  for (i = 0; i < lines.size(); i++) {
    if (i < lines.size() - 1)
      result += str_12432_2_12432_46793682;
    else
      result += str_12495_2_12495_46793871;
    if (i == 0)
      result += std::string(sr->get_id()) + str_5832_2_5832_1634370;
    result += lines[i];
    if (i < lines.size() - 1)
      result += str_92n_2_92n_56437;
  }

  return strdup(result.c_str());
}
#endif /* NOLUA */

/* Output a list of ports, compressing ranges like 80-85 */
static void output_rangelist_given_ports(int logt, const unsigned short *ports, int numports);

/* Prints the familiar Nmap tabular output showing the "interesting"
   ports found on the machine.  It also handles the Machine/Grepable
   output and the XML output.  It is pretty ugly -- in particular I
   should write helper functions to handle the table creation */
void printportoutput(const Target *currenths, const PortList *plist) {
  char protocol[MAX_IPPROTOSTRLEN + 1];
  char portinfo[64];
  char grepvers[256];
  char *p;
  const char *state;
  char serviceinfo[64];
  int i;
  int first = 1;
  const struct nprotoent *proto;
  Port *current;
  Port port;
  char hostname[1200];
  struct serviceDeductions sd;
  NmapOutputTable *Tbl = NULL;
  int portcol = -1;             // port or IP protocol #
  int statecol = -1;            // port/protocol state
  int servicecol = -1;          // service or protocol name
  int versioncol = -1;
  int reasoncol = -1;
  int colno = 0;
  unsigned int rowno;
  int numrows;
  int numignoredports = plist->numIgnoredPorts();
  int numports = plist->numPorts();
  state_reason_summary_t *reasons, *currentr;

  std::vector<const char *> saved_servicefps;

  if (o.noportscan || numports == 0)
    return;

  xml_start_tag(str_ports_5_ports_106854418);
  log_write(LOG_MACHINE, str_Host5_13_37s41_1184206444, currenths->targetipstr(),
            currenths->HostName());

  if ((o.verbose > 1 || o.debugging) && currenths->StartTime()) {
    time_t tm_secs, tm_sece;
    struct tm tm;
    int err;
    char tbufs[128];
    tm_secs = currenths->StartTime();
    tm_sece = currenths->EndTime();
    err = n_localtime(&tm_secs, &tm);
    if (err) {
      error(str_Error_22_3237s_857837152, strerror(err));
      log_write(LOG_PLAIN, str_Scann_18_ds92n_606544151,
          (long) (tm_sece - tm_secs));
    }
    else {
      if (strftime(tbufs, sizeof(tbufs), str_37Y45_20_3237Z_702900072, &tm) <= 0) {
        error(str_Unabl_41_2time_1752385545);
        log_write(LOG_PLAIN, str_Scann_18_ds92n_606544151,
            (long) (tm_sece - tm_secs));
      }
      else {
        log_write(LOG_PLAIN, str_Scann_24_ds92n_909527367,
            tbufs, (long) (tm_sece - tm_secs));
      }
    }
  }

  int prevstate = PORT_UNKNOWN;
  int istate;

  while ((istate = plist->nextIgnoredState(prevstate)) != PORT_UNKNOWN) {
    i = plist->getStateCounts(istate);
    xml_open_start_tag(str_extra_10_ports_763424258);
    xml_attribute(str_state_5_state_109757585, str_37s_2_37s_50831, statenum2str(istate));
    xml_attribute(str_count_5_count_94851343, str_37d_2_37d_50816, i);
    xml_close_start_tag();
    xml_newline();

    /* Show line like:
       Not shown: 98 open|filtered udp ports (no-response), 59 closed tcp ports (reset)
       if appropriate (note that states are reverse-sorted by # of ports) */
    if (prevstate == PORT_UNKNOWN) {
      // First time through, check special case
      if (numignoredports == numports) {
        log_write(LOG_PLAIN, str_All32_51_4692n_504791704,
            numignoredports, currenths->NameIP(hostname, sizeof(hostname)));
        log_write(LOG_MACHINE, str_92t37_6_s5832_1508248182, (o.ipprotscan) ? str_Proto_9_ocols_1404658875 : str_Ports_5_Ports_77301746);
        /* Grepable output supports only one ignored state. */
        if (plist->numIgnoredStates() == 1) {
          log_write(LOG_MACHINE, str_92tIg_24_37d41_1408966640, statenum2str(istate), i);
        }
      }
      log_write(LOG_PLAIN, str_Not32_11_n5832_1475516223);
    } else {
      log_write(LOG_PLAIN, str_4432_2_4432_1600735);
    }

    if((currentr = reasons = get_state_reason_summary(plist, istate)) == NULL) {
      log_write(LOG_PLAIN, str_37d32_10_7s37s_1630671345, i, statenum2str(istate),
          o.ipprotscan ? str_proto_8_tocol_989163880 : str_port_4_port_3446913,
          plist->getStateCounts(istate) == 1 ? "" : str_s_1_s_115);
      prevstate = istate;
      continue;
    }

    while(currentr != NULL) {
      if(currentr->count > 0) {
        xml_open_start_tag(str_extra_12_asons_684290207);
        xml_attribute(str_reaso_6_eason_934964668, str_37s_2_37s_50831, reason_str(currentr->reason_id, SINGULAR));
        xml_attribute(str_count_5_count_94851343, str_37d_2_37d_50816, currentr->count);
        xml_attribute(str_proto_5_proto_106940904, str_37s_2_37s_50831, IPPROTO2STR(currentr->proto));
        xml_write_raw(str_32por_9_19234_1335105352);
        output_rangelist_given_ports(LOG_XML, currentr->ports, currentr->count);
        xml_write_raw(str_9234_2_9234_1747770);
        xml_close_empty_tag();
        xml_newline();

        if (currentr != reasons)
          log_write(LOG_PLAIN, str_4432_2_4432_1600735);
        log_write(LOG_PLAIN, str_37d32_18_37s41_1798706866,
            currentr->count, statenum2str(istate), IPPROTO2STR(currentr->proto),
            o.ipprotscan ? str_proto_8_tocol_989163880 : str_port_4_port_3446913,
            plist->getStateCounts(istate) == 1 ? "" : str_s_1_s_115,
            reason_str(currentr->reason_id, SINGULAR));
      }
      currentr = currentr->next;
    }
    state_reason_summary_dinit(reasons);
    xml_end_tag();
    xml_newline();
    prevstate = istate;
  }

  log_write(LOG_PLAIN, str_92n_2_92n_56437);

  if (numignoredports == numports) {
    // Nothing left to show.
    xml_end_tag(); /* ports */
    xml_newline();
    log_flush_all();
    return;
  }

  /* OK, now it is time to deal with the service table ... */
  colno = 0;
  portcol = colno++;
  statecol = colno++;
  servicecol = colno++;
  if (o.reason)
    reasoncol = colno++;
  if (o.servicescan)
    versioncol = colno++;

  numrows = numports - numignoredports;

#ifndef NOLUA
  int scriptrows = 0;
  if (plist->numscriptresults > 0)
    scriptrows = plist->numscriptresults;
  numrows += scriptrows;
#endif

  assert(numrows > 0);
  numrows++; // The header counts as a row

  Tbl = new NmapOutputTable(numrows, colno);

  // Lets start with the headers
  if (o.ipprotscan)
    Tbl->addItem(0, portcol, false, str_PROTO_8_TOCOL_206537064, 8);
  else
    Tbl->addItem(0, portcol, false, str_PORT_4_PORT_2461825, 4);
  Tbl->addItem(0, statecol, false, str_STATE_5_STATE_79219825, 5);
  Tbl->addItem(0, servicecol, false, str_SERVI_7_RVICE_1592831339, 7);
  if (versioncol > 0)
    Tbl->addItem(0, versioncol, false, str_VERSI_7_RSION_1069590712, 7);
  if (reasoncol > 0)
    Tbl->addItem(0, reasoncol, false, str_REASO_6_EASON_1881635260, 6);

  log_write(LOG_MACHINE, str_92t37_6_s5832_1508248182, (o.ipprotscan) ? str_Proto_9_ocols_1404658875 : str_Ports_5_Ports_77301746);

  rowno = 1;
  if (o.ipprotscan) {
    current = NULL;
    while ((current = plist->nextPort(current, &port, IPPROTO_IP, 0)) != NULL) {
      if (!plist->isIgnoredState(current->state, NULL)) {
        if (!first)
          log_write(LOG_MACHINE, str_4432_2_4432_1600735);
        else
          first = 0;
        if (o.reason) {
          if (current->reason.ttl)
            Tbl->addItemFormatted(rowno, reasoncol, false, str_37s32_9_3237d_1938626691,
                                port_reason_str(current->reason), current->reason.ttl);
          else
            Tbl->addItem(rowno, reasoncol, true, port_reason_str(current->reason));
        }
        state = statenum2str(current->state);
        proto = nmap_getprotbynum(current->portno);
        Snprintf(portinfo, sizeof(portinfo), str_37s_2_37s_50831, proto ? proto->p_name : str_unkno_7_known_284840886);
        Tbl->addItemFormatted(rowno, portcol, false, str_37d_2_37d_50816, current->portno);
        Tbl->addItem(rowno, statecol, true, state);
        Tbl->addItem(rowno, servicecol, true, portinfo);
        log_write(LOG_MACHINE, str_37d47_9_37s47_1959233667, current->portno, state,
                  (proto) ? proto->p_name : "");
        xml_open_start_tag(str_port_4_port_3446913);
        xml_attribute(str_proto_8_tocol_989163880, str_ip_2_ip_3367);
        xml_attribute(str_porti_6_ortid_982480548, str_37d_2_37d_50816, current->portno);
        xml_close_start_tag();
        xml_open_start_tag(str_state_5_state_109757585);
        xml_attribute(str_state_5_state_109757585, str_37s_2_37s_50831, state);
        xml_attribute(str_reaso_6_eason_934964668, str_37s_2_37s_50831, reason_str(current->reason.reason_id, SINGULAR));
        xml_attribute(str_reaso_10_95ttl_900484492, str_37d_2_37d_50816, current->reason.ttl);
        if (current->reason.ip_addr.sockaddr.sa_family != AF_UNSPEC) {
          struct sockaddr_storage ss;
          memcpy(&ss, &current->reason.ip_addr, sizeof(current->reason.ip_addr));
          xml_attribute(str_reaso_9_n95ip_721784199, str_37s_2_37s_50831, inet_ntop_ez(&ss, sizeof(ss)));
        }
        xml_close_empty_tag();

        if (proto && proto->p_name && *proto->p_name) {
          xml_newline();
          xml_open_start_tag(str_servi_7_rvice_1984153269);
          xml_attribute(str_name_4_name_3373707, str_37s_2_37s_50831, proto->p_name);
          xml_attribute(str_conf_4_conf_3059492, str_8_1_8_56);
          xml_attribute(str_metho_6_ethod_1077554975, str_table_5_table_110115790);
          xml_close_empty_tag();
        }
        xml_end_tag(); /* port */
        xml_newline();
        rowno++;
      }
    }
  } else {
    char fullversion[160];

    current = NULL;
    while ((current = plist->nextPort(current, &port, TCPANDUDPANDSCTP, 0)) != NULL) {
      if (!plist->isIgnoredState(current->state, NULL)) {
        if (!first)
          log_write(LOG_MACHINE, str_4432_2_4432_1600735);
        else
          first = 0;
        strcpy(protocol, IPPROTO2STR(current->proto));
        Snprintf(portinfo, sizeof(portinfo), str_37d47_5_4737s_1125263700, current->portno, protocol);
        state = statenum2str(current->state);
        plist->getServiceDeductions(current->portno, current->proto, &sd);
        if (sd.service_fp && saved_servicefps.size() <= 8)
          saved_servicefps.push_back(sd.service_fp);

        current->getNmapServiceName(serviceinfo, sizeof(serviceinfo));

        Tbl->addItem(rowno, portcol, true, portinfo);
        Tbl->addItem(rowno, statecol, false, state);
        Tbl->addItem(rowno, servicecol, true, serviceinfo);
        if (o.reason) {
          if (current->reason.ttl)
            Tbl->addItemFormatted(rowno, reasoncol, false, str_37s32_9_3237d_1938626691,
                                  port_reason_str(current->reason), current->reason.ttl);
          else
            Tbl->addItem(rowno, reasoncol, true, port_reason_str(current->reason));
        }

        sd.populateFullVersionString(fullversion, sizeof(fullversion));
        if (*fullversion && versioncol > 0)
          Tbl->addItem(rowno, versioncol, true, fullversion);

        // How should we escape illegal chars in grepable output?
        // Well, a reasonably clean way would be backslash escapes
        // such as \/ and \\ .  // But that makes it harder to pick
        // out fields with awk, cut, and such.  So I'm gonna use the
        // ugly hack (fitting to grepable output) of replacing the '/'
        // character with '|' in the version field.
        Strncpy(grepvers, fullversion, sizeof(grepvers) / sizeof(*grepvers));
        p = grepvers;
        while ((p = strchr(p, '/'))) {
          *p = '|';
          p++;
        }
        if (sd.name || sd.service_fp || sd.service_tunnel != SERVICE_TUNNEL_NONE) {
          p = serviceinfo;
          while ((p = strchr(p, '/'))) {
            *p = '|';
            p++;
          }
        }
        else {
          serviceinfo[0] = '\0';
        }
        log_write(LOG_MACHINE, str_37d47_17_37s47_2000186269, current->portno,
                  state, protocol, serviceinfo, grepvers);

        xml_open_start_tag(str_port_4_port_3446913);
        xml_attribute(str_proto_8_tocol_989163880, str_37s_2_37s_50831, protocol);
        xml_attribute(str_porti_6_ortid_982480548, str_37d_2_37d_50816, current->portno);
        xml_close_start_tag();
        xml_open_start_tag(str_state_5_state_109757585);
        xml_attribute(str_state_5_state_109757585, str_37s_2_37s_50831, state);
        xml_attribute(str_reaso_6_eason_934964668, str_37s_2_37s_50831, reason_str(current->reason.reason_id, SINGULAR));
        xml_attribute(str_reaso_10_95ttl_900484492, str_37d_2_37d_50816, current->reason.ttl);
        if (current->reason.ip_addr.sockaddr.sa_family != AF_UNSPEC) {
          struct sockaddr_storage ss;
          memcpy(&ss, &current->reason.ip_addr, sizeof(current->reason.ip_addr));
          xml_attribute(str_reaso_9_n95ip_721784199, str_37s_2_37s_50831, inet_ntop_ez(&ss, sizeof(ss)));
        }
        xml_close_empty_tag();

        if (sd.name || sd.service_fp || sd.service_tunnel != SERVICE_TUNNEL_NONE)
          print_xml_service(&sd);

        rowno++;
#ifndef NOLUA
        if (o.script) {
          ScriptResults::const_iterator ssr_iter;
          for (ssr_iter = current->scriptResults.begin();
               ssr_iter != current->scriptResults.end(); ssr_iter++) {
            (*ssr_iter)->write_xml();

            char *script_output = formatScriptOutput((*ssr_iter));
            if (script_output != NULL) {
              Tbl->addItem(rowno, 0, true, true, script_output);
              free(script_output);
            }
            rowno++;
          }

        }
#endif

        xml_end_tag(); /* port */
        xml_newline();
      }
    }

  }
  /*  log_write(LOG_PLAIN,str_92n_2_92n_56437); */
  /* Grepable output supports only one ignored state. */
  if (plist->numIgnoredStates() == 1) {
    istate = plist->nextIgnoredState(PORT_UNKNOWN);
    if (plist->getStateCounts(istate) > 0)
      log_write(LOG_MACHINE, str_92tIg_24_37d41_1408966640,
                statenum2str(istate), plist->getStateCounts(istate));
  }
  xml_end_tag(); /* ports */
  xml_newline();

  if (o.defeat_rst_ratelimit && o.TCPScan() && plist->getStateCounts(PORT_FILTERED) > 0) {
    log_write(LOG_PLAIN, str_Some3_77_it92n_1528193449);
  }

  // Now we write the table for the user
  log_write(LOG_PLAIN, str_37s_2_37s_50831, Tbl->printableTable(NULL));
  delete Tbl;

  // There may be service fingerprints I would like the user to submit
  if (saved_servicefps.size() > 0) {
    int numfps = saved_servicefps.size();
    log_write(LOG_PLAIN, str_37d32_49_ata46_357108296
              str_32If3_61_owing_869952669
              str_32fin_17_s32at_1302087396
              str_32htt_52_5892n_1618793379,
              numfps, (numfps > 1) ? str_s_1_s_115 : "", (numfps > 1) ? str_s_1_s_115 : "");
    for (i = 0; i < numfps; i++) {
      if (numfps > 1)
        log_write(LOG_PLAIN, str_61616_76_6192n_2140802678);
      log_write(LOG_PLAIN, str_37s92_4_7s92n_1514362758, saved_servicefps[i]);
    }
  }
  log_flush_all();
}


/* MAX_STRFTIME_EXPANSION is the maximum length that a single %_ escape can
 * expand to, not including null terminator. If you add another supported
 * escape, check that it doesn't exceed this value, otherwise increase it.
 */
#define MAX_STRFTIME_EXPANSION 10
char *logfilename(const char *str, struct tm *tm) {
  char *ret, *end, *p;
  // Max expansion: str_37F_2_37F_50786 => str_YYYY4_10_m45dd_1223187358
  int retlen = strlen(str) * (MAX_STRFTIME_EXPANSION - 2) + 1;
  size_t written = 0;

  ret = (char *) safe_malloc(retlen);
  end = ret + retlen;

  for (p = ret; *str; str++) {
    if (*str == '%') {
      str++;
      written = 0;

      if (!*str)
        break;

#define FTIME_CASE(_fmt, _fmt_str) case _fmt: \
        written = strftime(p, end - p, _fmt_str, tm); \
        break;

      switch (*str) {
        FTIME_CASE('H', str_37H_2_37H_50788);
        FTIME_CASE('M', str_37M_2_37M_50793);
        FTIME_CASE('S', str_37S_2_37S_50799);
        FTIME_CASE('T', str_37H37_6_7M37S_368404170);
        FTIME_CASE('R', str_37H37_4_7H37M_1513076101);
        FTIME_CASE('m', str_37m_2_37m_50825);
        FTIME_CASE('d', str_37d_2_37d_50816);
        FTIME_CASE('y', str_37y_2_37y_50837);
        FTIME_CASE('Y', str_37Y_2_37Y_50805);
        FTIME_CASE('D', str_37m37_6_7d37y_1153012770);
        FTIME_CASE('F', str_37Y45_8_4537d_203782444);
      default:
        *p++ = *str;
        continue;
      }

      assert(end - p > 1);
      p += written;
    } else {
      *p++ = *str;
    }
  }

  *p = 0;

  return (char *) safe_realloc(ret, strlen(ret) + 1);
}

/* This is the workhorse of the logging functions.  Usually it is
   called through log_write(), but it can be called directly if you are dealing
   with a vfprintf-style va_list. YOU MUST SANDWICH EACH EXECUTION OF THIS CALL
   BETWEEN va_start() AND va_end() calls. */
void log_vwrite(int logt, const char *fmt, va_list ap) {
  char *writebuf;
  bool skid_noxlate = false;
  int rc = 0;
  int len;
  int fileidx = 0;
  int l;
  int logtype;
  va_list apcopy;

  for (logtype = 1; logtype <= LOG_MAX; logtype <<= 1) {

    if (!(logt & logtype))
      continue;

    switch (logtype) {
      case LOG_STDOUT:
        vfprintf(o.nmap_stdout, fmt, ap);
        break;

      case LOG_STDERR:
        fflush(stdout); // Otherwise some systems will print stderr out of order
        vfprintf(stderr, fmt, ap);
        break;

      case LOG_SKID_NOXLT:
        skid_noxlate = true;
        /* no break */
      case LOG_NORMAL:
      case LOG_MACHINE:
      case LOG_SKID:
      case LOG_XML:
        if (logtype == LOG_SKID_NOXLT)
            l = LOG_SKID;
        else
            l = logtype;
        fileidx = 0;
        while ((l & 1) == 0) {
          fileidx++;
          l >>= 1;
        }
        assert(fileidx < LOG_NUM_FILES);
        if (o.logfd[fileidx]) {
          len = alloc_vsprintf(&writebuf, fmt, ap);
          if (writebuf == NULL)
            fatal(str_37s58_26_led46_1100544084, __func__);
          if (len) {
            if ((logtype & (LOG_SKID|LOG_SKID_NOXLT)) && !skid_noxlate)
              skid_output(writebuf);

            rc = fwrite(writebuf, len, 1, o.logfd[fileidx]);
            if (rc != 1) {
              fatal(str_Faile_85_ing46_961889174, len, logtype, rc);
            }
            va_end(apcopy);
          }
          free(writebuf);
        }
        break;

      default:
        /* Unknown log type.
         * ---
         * Note that we're not calling fatal() here to avoid infinite call loop
         * between fatal() and this log_vwrite() function. */
        assert(0); /* We want people to report it. */
    }
  }

  return;
}

/* Write some information (printf style args) to the given log stream(s).
 Remember to watch out for format string bugs.  */
void log_write(int logt, const char *fmt, ...) {
  va_list ap;
  assert(logt > 0);

  if (!fmt || !*fmt)
    return;

  for (int l = 1; l <= LOG_MAX; l <<= 1) {
    if (logt & l) {
      va_start(ap, fmt);
      log_vwrite(l, fmt, ap);
      va_end(ap);
    }
  }
  return;
}

/* Close the given log stream(s) */
void log_close(int logt) {
  int i;
  if (logt < 0 || logt > LOG_FILE_MASK)
    return;
  for (i = 0; logt; logt >>= 1, i++)
    if (o.logfd[i] && (logt & 1))
      fclose(o.logfd[i]);
}

/* Flush the given log stream(s).  In other words, all buffered output
   is written to the log immediately */
void log_flush(int logt) {
  int i;

  if (logt & LOG_STDOUT) {
    fflush(o.nmap_stdout);
    logt -= LOG_STDOUT;
  }

  if (logt & LOG_STDERR) {
    fflush(stderr);
    logt -= LOG_STDERR;
  }

  if (logt & LOG_SKID_NOXLT)
    fatal(str_You32_47_NOXLT_98289128, __func__);

  if (logt < 0 || logt > LOG_FILE_MASK)
    return;

  for (i = 0; logt; logt >>= 1, i++) {
    if (!o.logfd[i] || !(logt & 1))
      continue;
    fflush(o.logfd[i]);
  }

}

/* Flush every single log stream -- all buffered output is written to the
   corresponding logs immediately */
void log_flush_all() {
  int fileno;

  for (fileno = 0; fileno < LOG_NUM_FILES; fileno++) {
    if (o.logfd[fileno])
      fflush(o.logfd[fileno]);
  }
  fflush(stdout);
  fflush(stderr);
}

/* Open a log descriptor of the type given to the filename given.  If
   append is true, the file will be appended instead of clobbered if
   it already exists.  If the file does not exist, it will be created */
int log_open(int logt, bool append, const char *filename) {
  int i = 0;
  if (logt <= 0 || logt > LOG_FILE_MASK)
    return -1;
  while ((logt & 1) == 0) {
    i++;
    logt >>= 1;
  }
  if (o.logfd[i])
    fatal(str_Only3_35_lowed_1938653486, logtypes[i]);
  if (*filename == '-' && *(filename + 1) == '\0') {
    o.logfd[i] = stdout;
    o.nmap_stdout = fopen(DEVNULL, str_w_1_w_119);
    if (!o.nmap_stdout)
      pfatal(str_Could_41_iting_2039589606, DEVNULL);
  } else {
    if (append)
      o.logfd[i] = fopen(filename, str_a_1_a_97);
    else
      o.logfd[i] = fopen(filename, str_w_1_w_119);
    if (!o.logfd[i])
      pfatal(str_Faile_44_iting_1172901720, logtypes[i],
            filename);
  }
  return 1;
}


/* The items in ports should be
   in sequential order for space savings and easier to read output.  Outputs the
   rangelist to the log stream given (such as LOG_MACHINE or LOG_XML) */
static void output_rangelist_given_ports(int logt, const unsigned short *ports,
                                         int numports) {
  int start, end;

  start = 0;
  while (start < numports) {
    end = start;
    while (end + 1 < numports && ports[end + 1] == ports[end] + 1)
      end++;
    if (start > 0)
      log_write(logt, str_44_1_44_1664);
    if (start == end)
      log_write(logt, str_37hu_3_37hu_1575537, ports[start]);
    else
      log_write(logt, str_37hu4_7_537hu_1103600163, ports[start], ports[end]);
    start = end + 1;
  }
}

/* Output the list of ports scanned to the top of machine parseable
   logs (in a comment, unfortunately).  The items in ports should be
   in sequential order for space savings and easier to read output */
void output_ports_to_machine_parseable_output(const struct scan_lists *ports) {
  int tcpportsscanned = ports->tcp_count;
  int udpportsscanned = ports->udp_count;
  int sctpportsscanned = ports->sctp_count;
  int protsscanned = ports->prot_count;
  log_write(LOG_MACHINE, str_3532P_24_37d59_789020809, tcpportsscanned);
  if (tcpportsscanned)
    output_rangelist_given_ports(LOG_MACHINE, ports->tcp_ports, tcpportsscanned);
  log_write(LOG_MACHINE, str_4132U_9_37d59_766768131, udpportsscanned);
  if (udpportsscanned)
    output_rangelist_given_ports(LOG_MACHINE, ports->udp_ports, udpportsscanned);
  log_write(LOG_MACHINE, str_4132S_10_37d59_426206528, sctpportsscanned);
  if (sctpportsscanned)
    output_rangelist_given_ports(LOG_MACHINE, ports->sctp_ports, sctpportsscanned);
  log_write(LOG_MACHINE, str_4132P_15_37d59_289763497, protsscanned);
  if (protsscanned)
    output_rangelist_given_ports(LOG_MACHINE, ports->prots, protsscanned);
  log_write(LOG_MACHINE, str_4192n_3_4192n_49539288);
  log_flush_all();
}

// A simple helper function for doscaninfo handles the c14n of o.scanflags
static void doscanflags() {
  struct {
    unsigned char flag;
    const char *name;
  } flags[] = {
    { TH_FIN, str_FIN_3_FIN_69611 },
    { TH_SYN, str_SYN_3_SYN_82600 },
    { TH_RST, str_RST_3_RST_81459 },
    { TH_PUSH, str_PSH_3_PSH_79525 },
    { TH_ACK, str_ACK_3_ACK_64617 },
    { TH_URG, str_URG_3_URG_84298 },
    { TH_ECE, str_ECE_3_ECE_68455 },
    { TH_CWR, str_CWR_3_CWR_67166 }
  };

  if (o.scanflags != -1) {
    std::string flagstring;

    for (unsigned int i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
      if (o.scanflags & flags[i].flag)
        flagstring += flags[i].name;
    }
    xml_attribute(str_scanf_9_flags_1819038166, str_37s_2_37s_50831, flagstring.c_str());
  }
}

/* Simple helper function for output_xml_scaninfo_records */
static void doscaninfo(const char *type, const char *proto,
                       const unsigned short *ports, int numports) {
  xml_open_start_tag(str_scani_8_ninfo_889871189);
  xml_attribute(str_type_4_type_3575610, str_37s_2_37s_50831, type);
  if (strncmp(proto, str_tcp_3_tcp_114657, 3) == 0) {
    doscanflags();
  }
  xml_attribute(str_proto_8_tocol_989163880, str_37s_2_37s_50831, proto);
  xml_attribute(str_numse_11_vices_321770980, str_37d_2_37d_50816, numports);
  xml_write_raw(str_32ser_12_19234_1030774158);
  output_rangelist_given_ports(LOG_XML, ports, numports);
  xml_write_raw(str_9234_2_9234_1747770);
  xml_close_empty_tag();
  xml_newline();
}

static std::string quote(const char *s) {
  std::string result("");
  const char *p;
  bool space;

  space = false;
  for (p = s; *p != '\0'; p++) {
    if (isspace(*p))
      space = true;
    if (*p == '"' || *p == '\\')
      result += str_9292_2_9292_1747954;
    result += *p;
  }

  if (space)
    result = str_9234_2_9234_1747770 + result + str_9234_2_9234_1747770;

  return result;
}

/* Return a std::string containing all n strings separated by whitespace, and
   individually quoted if needed. */
std::string join_quoted(const char * const strings[], unsigned int n) {
  std::string result("");
  unsigned int i;

  for (i = 0; i < n; i++) {
    if (i > 0)
      result += str_32_1_32_1631;
    result += quote(strings[i]);
  }

  return result;
}

/* Similar to output_ports_to_machine_parseable_output, this function
   outputs the XML version, which is scaninfo records of each scan
   requested and the ports which it will scan for */
void output_xml_scaninfo_records(const struct scan_lists *scanlist) {
  if (o.synscan)
    doscaninfo(str_syn_3_syn_114376, str_tcp_3_tcp_114657, scanlist->tcp_ports, scanlist->tcp_count);
  if (o.ackscan)
    doscaninfo(str_ack_3_ack_96393, str_tcp_3_tcp_114657, scanlist->tcp_ports, scanlist->tcp_count);
  if (o.bouncescan)
    doscaninfo(str_bounc_6_ounce_1383205240, str_tcp_3_tcp_114657, scanlist->tcp_ports, scanlist->tcp_count);
  if (o.connectscan)
    doscaninfo(str_conne_7_nnect_951351530, str_tcp_3_tcp_114657, scanlist->tcp_ports, scanlist->tcp_count);
  if (o.nullscan)
    doscaninfo(str_null_4_null_3392903, str_tcp_3_tcp_114657, scanlist->tcp_ports, scanlist->tcp_count);
  if (o.xmasscan)
    doscaninfo(str_xmas_4_xmas_3682791, str_tcp_3_tcp_114657, scanlist->tcp_ports, scanlist->tcp_count);
  if (o.windowscan)
    doscaninfo(str_windo_6_indow_787751952, str_tcp_3_tcp_114657, scanlist->tcp_ports, scanlist->tcp_count);
  if (o.maimonscan)
    doscaninfo(str_maimo_6_aimon_1081571945, str_tcp_3_tcp_114657, scanlist->tcp_ports, scanlist->tcp_count);
  if (o.finscan)
    doscaninfo(str_fin_3_fin_101387, str_tcp_3_tcp_114657, scanlist->tcp_ports, scanlist->tcp_count);
  if (o.udpscan)
    doscaninfo(str_udp_3_udp_115649, str_udp_3_udp_115649, scanlist->udp_ports, scanlist->udp_count);
  if (o.sctpinitscan)
    doscaninfo(str_sctpi_8_pinit_344070180, str_sctp_4_sctp_3524812, scanlist->sctp_ports, scanlist->sctp_count);
  if (o.sctpcookieechoscan)
    doscaninfo(str_sctpc_14_eecho_172630955, str_sctp_4_sctp_3524812, scanlist->sctp_ports, scanlist->sctp_count);
  if (o.ipprotscan)
    doscaninfo(str_ippro_7_proto_2012011809, str_ip_2_ip_3367, scanlist->prots, scanlist->prot_count);
  log_flush_all();
}

/* Prints the MAC address (if discovered) to XML output */
static void print_MAC_XML_Info(const Target *currenths) {
  const u8 *mac = currenths->MACAddress();
  char macascii[32];

  if (mac) {
    const char *macvendor = MACPrefix2Corp(mac);
    Snprintf(macascii, sizeof(macascii), str_3702X_29_3702X_877469763,
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    xml_open_start_tag(str_addre_7_dress_1147692044);
    xml_attribute(str_addr_4_addr_2989041, str_37s_2_37s_50831, macascii);
    xml_attribute(str_addrt_8_rtype_1218262357, str_mac_3_mac_107855);
    if (macvendor)
      xml_attribute(str_vendo_6_endor_820075192, str_37s_2_37s_50831, macvendor);
    xml_close_empty_tag();
    xml_newline();
  }
}

/* Helper function to write the status and address/hostname info of a host
   into the XML log */
static void write_xml_initial_hostinfo(const Target *currenths,
                                       const char *status) {
  xml_open_start_tag(str_statu_6_tatus_892481550);
  xml_attribute(str_state_5_state_109757585, str_37s_2_37s_50831, status);
  xml_attribute(str_reaso_6_eason_934964668, str_37s_2_37s_50831, reason_str(currenths->reason.reason_id, SINGULAR));
  xml_attribute(str_reaso_10_95ttl_900484492, str_37d_2_37d_50816, currenths->reason.ttl);
  xml_close_empty_tag();
  xml_newline();
  xml_open_start_tag(str_addre_7_dress_1147692044);
  xml_attribute(str_addr_4_addr_2989041, str_37s_2_37s_50831, currenths->targetipstr());
  xml_attribute(str_addrt_8_rtype_1218262357, str_37s_2_37s_50831, (o.af() == AF_INET) ? str_ipv4_4_ipv4_3239397 : str_ipv6_4_ipv6_3239399);
  xml_close_empty_tag();
  xml_newline();
  print_MAC_XML_Info(currenths);
  /* Output a hostnames element whenever we have a name to write or the target
     is up. */
  if (currenths->TargetName() != NULL || *currenths->HostName() || strcmp(status, str_up_2_up_3739) == 0) {
    xml_start_tag(str_hostn_9_names_703976800);
    xml_newline();
    if (currenths->TargetName() != NULL) {
      xml_open_start_tag(str_hostn_8_tname_299803597);
      xml_attribute(str_name_4_name_3373707, str_37s_2_37s_50831, currenths->TargetName());
      xml_attribute(str_type_4_type_3575610, str_user_4_user_3599307);
      xml_close_empty_tag();
      xml_newline();
    }
    if (*currenths->HostName()) {
      xml_open_start_tag(str_hostn_8_tname_299803597);
      xml_attribute(str_name_4_name_3373707, str_37s_2_37s_50831, currenths->HostName());
      xml_attribute(str_type_4_type_3575610, str_PTR_3_PTR_79566);
      xml_close_empty_tag();
      xml_newline();
    }
    xml_end_tag();
    xml_newline();
  }
  log_flush_all();
}

void write_xml_hosthint(const Target *currenths) {
  xml_start_tag(str_hosth_8_thint_299974609);
  write_xml_initial_hostinfo(currenths, (currenths->flags & HOST_UP) ? str_up_2_up_3739 : str_down_4_down_3089570);
  xml_end_tag();
  xml_newline();
  log_flush_all();
}

static void write_xml_osclass(const OS_Classification *osclass, double accuracy) {
  xml_open_start_tag(str_oscla_7_class_1179211244);
  xml_attribute(str_type_4_type_3575610, str_37s_2_37s_50831, osclass->Device_Type);
  xml_attribute(str_vendo_6_endor_820075192, str_37s_2_37s_50831, osclass->OS_Vendor);
  xml_attribute(str_osfam_8_amily_2119733688, str_37s_2_37s_50831, osclass->OS_Family);
  // Because the OS_Generation field is optional.
  if (osclass->OS_Generation)
    xml_attribute(str_osgen_5_osgen_106039020, str_37s_2_37s_50831, osclass->OS_Generation);
  xml_attribute(str_accur_8_uracy_2131707655, str_37d_2_37d_50816, (int) (accuracy * 100));
  if (osclass->cpe.empty()) {
    xml_close_empty_tag();
  } else {
    unsigned int i;

    xml_close_start_tag();
    for (i = 0; i < osclass->cpe.size(); i++) {
      xml_start_tag(str_cpe_3_cpe_98712);
      xml_write_escaped(str_37s_2_37s_50831, osclass->cpe[i]);
      xml_end_tag();
    }
    xml_end_tag();
  }
  xml_newline();
}

static void write_xml_osmatch(const FingerMatch *match, double accuracy) {
  xml_open_start_tag(str_osmat_7_match_1170285983);
  xml_attribute(str_name_4_name_3373707, str_37s_2_37s_50831, match->OS_name);
  xml_attribute(str_accur_8_uracy_2131707655, str_37d_2_37d_50816, (int) (accuracy * 100));
  xml_attribute(str_line_4_line_3321844, str_37d_2_37d_50816, match->line);
  /* When o.deprecated_xml_osclass is true, we don't write osclass elements as
     children of osmatch but rather as unrelated siblings. */
  if (match->OS_class.empty() || o.deprecated_xml_osclass) {
    xml_close_empty_tag();
  } else {
    unsigned int i;

    xml_close_start_tag();
    xml_newline();
    for (i = 0; i < match->OS_class.size(); i++)
      write_xml_osclass(&match->OS_class[i], accuracy);
    xml_end_tag();
  }
  xml_newline();
}

/* Convert a number to a string, keeping the given number of significant digits.
   The result is returned in a static buffer. */
static char *num_to_string_sigdigits(double d, int digits) {
  static char buf[32];
  int shift;
  int n;

  assert(digits >= 0);
  if (d == 0.0) {
    shift = -digits;
  } else {
    shift = (int) floor(log10(fabs(d))) - digits + 1;
    d = floor(d / pow(10.0, shift) + 0.5);
    d = d * pow(10.0, shift);
  }

  n = Snprintf(buf, sizeof(buf), str_37464_4_4642f_357665790, MAX(0, -shift), d);
  assert(n > 0 && n < (int) sizeof(buf));

  return buf;
}

/* Writes a heading for a full scan report ("Nmap scan report for..."),
   including host status and DNS records. */
void write_host_header(const Target *currenths) {
  if ((currenths->flags & HOST_UP) || o.verbose || o.always_resolve) {
    if (currenths->flags & HOST_UP) {
      log_write(LOG_PLAIN, str_Nmap3_25_7s92n_921517650, currenths->NameIP());
    } else if (currenths->flags & HOST_DOWN) {
      log_write(LOG_PLAIN, str_Nmap3_34_2down_1062848989, currenths->NameIP());
      if (o.reason)
        log_write(LOG_PLAIN, str_44323_4_3237s_442906960, target_reason_str(currenths));
      log_write(LOG_PLAIN, str_9392n_3_9392n_54216475);
    }
  }
  write_host_status(currenths);
  if (currenths->TargetName() != NULL
      && !currenths->unscanned_addrs.empty()) {

    log_write(LOG_PLAIN, str_Other_37_d4158_2108139470,
      currenths->TargetName());
    for (std::list<struct sockaddr_storage>::const_iterator it = currenths->unscanned_addrs.begin(), end = currenths->unscanned_addrs.end();
        it != end; it++) {
      struct sockaddr_storage ss = *it;
      log_write(LOG_PLAIN, str_3237s_3_3237s_48639952, inet_ntop_ez(&ss, sizeof(ss)));
    }
    log_write(LOG_PLAIN, str_92n_2_92n_56437);
  }
  /* Print reverse DNS if it differs. */
  if (currenths->TargetName() != NULL
      && currenths->HostName() != NULL && currenths->HostName()[0] != '\0'
      && strcmp(currenths->TargetName(), currenths->HostName()) != 0) {
    log_write(LOG_PLAIN, str_rDNS3_24_7s92n_367392491,
      currenths->targetipstr(), currenths->HostName());
  }
}

/* Writes host status info to the log streams (including STDOUT).  An
   example is "Host: 10.11.12.13 (foo.bar.example.com)\tStatus: Up\n" to
   machine log. */
void write_host_status(const Target *currenths) {
  if (o.listscan) {
    /* write str_unkno_7_known_284840886 to machine and xml */
    log_write(LOG_MACHINE, str_Host5_32_wn92n_520801426,
              currenths->targetipstr(), currenths->HostName());
    write_xml_initial_hostinfo(currenths, str_unkno_7_known_284840886);
  } else if (currenths->weird_responses) {
    /* SMURF ADDRESS */
    /* Write xml str_down_4_down_3089570 or str_up_2_up_3739 based on flags and the smurf info */
    write_xml_initial_hostinfo(currenths,
                               (currenths->
                                flags & HOST_UP) ? str_up_2_up_3739 : str_down_4_down_3089570);
    xml_open_start_tag(str_smurf_5_smurf_109568207);
    xml_attribute(str_respo_9_onses_1960086446, str_37d_2_37d_50816, currenths->weird_responses);
    xml_close_empty_tag();
    xml_newline();
    log_write(LOG_MACHINE, str_Host5_45_4192n_1393366914,
              currenths->targetipstr(), currenths->HostName(),
              currenths->weird_responses);

    if (o.noportscan) {
      log_write(LOG_PLAIN, str_Host3_74_7s92n_1793396330,
                currenths->weird_responses,
                (currenths->flags & HOST_UP) ? str_32Not_38_ded46_702511906 : "");
    } else {
      log_write(LOG_PLAIN, str_Host3_76_4692n_1609106887,
                currenths->weird_responses,
                (currenths->flags & HOST_UP) ? str_32Sti_55_n32IP_1689631946 : str_Skipp_13_2host_757462872);
    }
  } else {
    /* Ping scan / port scan. */

    write_xml_initial_hostinfo(currenths, (currenths->flags & HOST_UP) ? str_up_2_up_3739 : str_down_4_down_3089570);
    if (currenths->flags & HOST_UP) {
      log_write(LOG_PLAIN, str_Host3_10_s32up_1703741003);
      if (o.reason)
        log_write(LOG_PLAIN, str_44323_4_3237s_442906960, target_reason_str(currenths));
      if (o.reason && currenths->reason.ttl)
        log_write(LOG_PLAIN, str_32ttl_7_3237d_1703671988, currenths->reason.ttl);
      if (currenths->to.srtt != -1)
        log_write(LOG_PLAIN, str_32403_14_ncy41_106022845,
                  num_to_string_sigdigits(currenths->to.srtt / 1000000.0, 2));
      log_write(LOG_PLAIN, str_4692n_3_4692n_49688243);

      log_write(LOG_MACHINE, str_Host5_27_Up92n_486129303,
                currenths->targetipstr(), currenths->HostName());
    } else if (currenths->flags & HOST_DOWN) {
      log_write(LOG_MACHINE, str_Host5_29_wn92n_1141498864,
                currenths->targetipstr(), currenths->HostName());
    }
  }
}

/* Returns -1 if adding the entry is not possible because it would
   overflow.  Otherwise it returns the new number of entries.  Note
   that only unique entries are added.  Also note that *numentries is
   incremented if the candidate is added.  arrsize is the number of
   char * members that fit into arr */
static int addtochararrayifnew(const char *arr[], int *numentries, int arrsize,
                               const char *candidate) {
  int i;

  // First lets see if the member already exists
  for (i = 0; i < *numentries; i++) {
    if (strcmp(arr[i], candidate) == 0)
      return *numentries;
  }

  // Not already there... do we have room for a new one?
  if (*numentries >= arrsize)
    return -1;

  // OK, not already there and we have room, so we'll add it.
  arr[*numentries] = candidate;
  (*numentries)++;
  return *numentries;
}

/* guess is true if we should print guesses */
#define MAX_OS_CLASSMEMBERS 8
static void printosclassificationoutput(const struct
                                        OS_Classification_Results *OSR,
                                        bool guess) {
  int classno, cpeno, familyno;
  unsigned int i;
  int overflow = 0;             /* Whether we have too many devices to list */
  const char *types[MAX_OS_CLASSMEMBERS];
  const char *cpes[MAX_OS_CLASSMEMBERS];
  char fullfamily[MAX_OS_CLASSMEMBERS][128];    // str_91ven_20_ily93_798039666
  double familyaccuracy[MAX_OS_CLASSMEMBERS];   // highest accuracy for this fullfamily
  char familygenerations[MAX_OS_CLASSMEMBERS][96];      // example: str_446X1_11_4646X_266953665
  int numtypes = 0, numcpes = 0, numfamilies = 0;
  char tmpbuf[1024];

  for (i = 0; i < MAX_OS_CLASSMEMBERS; i++) {
    familygenerations[i][0] = '\0';
    familyaccuracy[i] = 0.0;
  }

  if (OSR->overall_results == OSSCAN_SUCCESS) {

    if (o.deprecated_xml_osclass) {
      for (classno = 0; classno < OSR->OSC_num_matches; classno++)
        write_xml_osclass(OSR->OSC[classno], OSR->OSC_Accuracy[classno]);
    }

    // Now to create the fodder for normal output
    for (classno = 0; classno < OSR->OSC_num_matches; classno++) {
      /* We have processed enough if any of the following are true */
      if ((!guess && classno >= OSR->OSC_num_perfect_matches) ||
          OSR->OSC_Accuracy[classno] <= OSR->OSC_Accuracy[0] - 0.1 ||
          (OSR->OSC_Accuracy[classno] < 1.0 && classno > 9))
        break;
      if (addtochararrayifnew(types, &numtypes, MAX_OS_CLASSMEMBERS,
                              OSR->OSC[classno]->Device_Type) == -1) {
        overflow = 1;
      }
      for (i = 0; i < OSR->OSC[classno]->cpe.size(); i++) {
        if (addtochararrayifnew(cpes, &numcpes, MAX_OS_CLASSMEMBERS,
                                OSR->OSC[classno]->cpe[i]) == -1) {
          overflow = 1;
        }
      }

      // If family and vendor names are the same, no point being redundant
      if (strcmp(OSR->OSC[classno]->OS_Vendor, OSR->OSC[classno]->OS_Family) == 0)
        Strncpy(tmpbuf, OSR->OSC[classno]->OS_Family, sizeof(tmpbuf));
      else
        Snprintf(tmpbuf, sizeof(tmpbuf), str_37s32_5_3237s_696898911, OSR->OSC[classno]->OS_Vendor, OSR->OSC[classno]->OS_Family);


      // Let's see if it is already in the array
      for (familyno = 0; familyno < numfamilies; familyno++) {
        if (strcmp(fullfamily[familyno], tmpbuf) == 0) {
          // got a match ... do we need to add the generation?
          if (OSR->OSC[classno]->OS_Generation
              && !strstr(familygenerations[familyno],
                         OSR->OSC[classno]->OS_Generation)) {
            int flen = strlen(familygenerations[familyno]);
            // We add it, preceded by | if something is already there
            if (flen + 2 + strlen(OSR->OSC[classno]->OS_Generation) >=
                sizeof(familygenerations[familyno]))
              fatal(str_buffe_36_tions_2138512445);
            if (*familygenerations[familyno])
              strcat(familygenerations[familyno], str_124_1_124_48691);
            strncat(familygenerations[familyno],
                    OSR->OSC[classno]->OS_Generation,
                    sizeof(familygenerations[familyno]) - flen - 1);
          }
          break;
        }
      }

      if (familyno == numfamilies) {
        // Looks like the new family is not in the list yet.  Do we have room to add it?
        if (numfamilies >= MAX_OS_CLASSMEMBERS) {
          overflow = 1;
          break;
        }
        // Have space, time to add...
        Strncpy(fullfamily[numfamilies], tmpbuf, 128);
        if (OSR->OSC[classno]->OS_Generation)
          Strncpy(familygenerations[numfamilies],
                  OSR->OSC[classno]->OS_Generation, 48);
        familyaccuracy[numfamilies] = OSR->OSC_Accuracy[classno];
        numfamilies++;
      }
    }

    if (!overflow && numfamilies >= 1) {
      log_write(LOG_PLAIN, str_Devic_13_e5832_2111207633);
      for (classno = 0; classno < numtypes; classno++)
        log_write(LOG_PLAIN, str_37s37_4_7s37s_1514357152, types[classno], (classno < numtypes - 1) ? str_124_1_124_48691 : "");
      log_write(LOG_PLAIN, str_92nRu_13_s5832_446442361, OSR->OSC_num_perfect_matches == 0 ? str_3240J_16_ING41_909320206 : "");
      for (familyno = 0; familyno < numfamilies; familyno++) {
        if (familyno > 0)
          log_write(LOG_PLAIN, str_4432_2_4432_1600735);
        log_write(LOG_PLAIN, str_37s_2_37s_50831, fullfamily[familyno]);
        if (*familygenerations[familyno])
          log_write(LOG_PLAIN, str_3237s_3_3237s_48639952, familygenerations[familyno]);
        if (familyno >= OSR->OSC_num_perfect_matches)
          log_write(LOG_PLAIN, str_32403_8_73741_2104319606,
                    floor(familyaccuracy[familyno] * 100));
      }
      log_write(LOG_PLAIN, str_92n_2_92n_56437);

      if (numcpes > 0) {
        log_write(LOG_PLAIN, str_OS32C_7_CPE58_1378187448);
        for (cpeno = 0; cpeno < numcpes; cpeno++)
          log_write(LOG_PLAIN, str_3237s_3_3237s_48639952, cpes[cpeno]);
        log_write(LOG_PLAIN, str_92n_2_92n_56437);
      }
    }
  }
  log_flush_all();
  return;
}

/* Prints the MAC address if one was found for the target (generally
   this means that the target is directly connected on an ethernet
   network.  This only prints to human output -- XML is handled by a
   separate call ( print_MAC_XML_Info ) because it needs to be printed
   in a certain place to conform to DTD. */
void printmacinfo(const Target *currenths) {
  const u8 *mac = currenths->MACAddress();
  char macascii[32];

  if (mac) {
    const char *macvendor = MACPrefix2Corp(mac);
    Snprintf(macascii, sizeof(macascii), str_3702X_29_3702X_877469763,
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    log_write(LOG_PLAIN, str_MAC32_22_4192n_164101301, macascii,
              macvendor ? macvendor : str_Unkno_7_known_1379812394);
  }
}



/* A convenience wrapper around mergeFPs. */
const char *FingerPrintResultsIPv4::merge_fpr(const Target *currenths,
                             bool isGoodFP, bool wrapit) const {
  return mergeFPs(this->FPs, this->numFPs, isGoodFP, currenths->TargetSockAddr(),
                  currenths->distance,
                  currenths->distance_calculation_method,
                  currenths->MACAddress(), this->osscan_opentcpport,
                  this->osscan_closedtcpport, this->osscan_closedudpport,
                  wrapit);
}

/* Run-length encode a string in chunks of two bytes. The output sequence
   AA{n} means to repeat AA n times. The input must not contain '{' or '}'
   characters. */
static std::string run_length_encode(const std::string &s) {
  std::ostringstream result;
  const char *p, *q;
  unsigned int reps;

  p = s.c_str();
  while (*p != '\0' && *(p + 1) != '\0') {
    for (q = p + 2; *q == *p && *(q + 1) == *(p + 1); q += 2)
      ;
    reps = (q - p) / 2;
    if (reps < 3)
      result << std::string(p, q);
    else
      result << std::string(p, 2) << str_123_1_123_48690 << reps << str_125_1_125_48692;
    p = q;
  }
  if (*p != '\0')
    result << std::string(p);

  return result.str();
}

static std::string wrap(const std::string &s) {
  const static char *prefix = str_OS58_3_OS58_2434951;
  std::string t, buf;
  int i, len, prefixlen;
  size_t p;

  t = s;
  /* Remove newlines. */
  p = 0;
  while ((p = t.find(str_92n_2_92n_56437, p)) != std::string::npos)
    t.erase(p, 1);

  len = t.size();
  prefixlen = strlen(prefix);
  assert(FP_RESULT_WRAP_LINE_LEN > prefixlen);
  for (i = 0; i < len; i += FP_RESULT_WRAP_LINE_LEN - prefixlen) {
    buf.append(prefix);
    buf.append(t, i, FP_RESULT_WRAP_LINE_LEN - prefixlen);
    buf.append(str_92n_2_92n_56437);
  }

  return buf;
}

static void scrub_packet(PacketElement *pe, unsigned char fill) {
  unsigned char fillbuf[16];

  memset(fillbuf, fill, sizeof(fillbuf));
  for (; pe != NULL; pe = pe->getNextElement()) {
    if (pe->protocol_id() == HEADER_TYPE_IPv6) {
      IPv6Header *ipv6 = (IPv6Header *) pe;
      ipv6->setSourceAddress(fillbuf);
      ipv6->setDestinationAddress(fillbuf);
    } else if (pe->protocol_id() == HEADER_TYPE_ICMPv6) {
      ICMPv6Header *icmpv6 = (ICMPv6Header *) pe;
      in6_addr *addr = (in6_addr *) fillbuf;
      if (icmpv6->getType() == ICMPV6_NEIGHBOR_ADVERTISEMENT)
        icmpv6->setTargetAddress(*addr);
    }
  }
}

static std::string get_scrubbed_buffer(const FPResponse *resp) {
  std::ostringstream result;
  PacketElement *scrub1, *scrub2;
  u8 *buf1, *buf2;
  int len1, len2;
  unsigned int i;

  scrub1 = PacketParser::split(resp->buf, resp->len);
  assert(scrub1 != NULL);
  scrub_packet(scrub1, 0x00);

  scrub2 = PacketParser::split(resp->buf, resp->len);
  assert(scrub2 != NULL);
  scrub_packet(scrub2, 0xFF);

  buf1 = scrub1->getBinaryBuffer(&len1);
  buf2 = scrub2->getBinaryBuffer(&len2);

  assert(resp->len == (unsigned int) len1);
  assert(resp->len == (unsigned int) len2);

  result.fill('0');
  result << std::hex;
  for (i = 0; i < resp->len; i++) {
    if (resp->buf[i] == buf1[i] && resp->buf[i] == buf2[i]) {
      result.width(2);
      result << (unsigned int) resp->buf[i];
    } else {
      result << str_XX_2_XX_2816;
    }
  }

  free(buf1);
  free(buf2);
  PacketParser::freePacketChain(scrub1);
  PacketParser::freePacketChain(scrub2);

  return result.str();
}

const char *FingerPrintResultsIPv6::merge_fpr(const Target *currenths,
                             bool isGoodFP, bool wrapit) const {
  static char str[10240];
  const FingerPrintResultsIPv6 *FPR;
  std::ostringstream result;
  std::string output;
  unsigned int i;

  /* Write the SCAN line. */
  WriteSInfo(str, sizeof(str), isGoodFP, str_6_1_6_54, currenths->TargetSockAddr(),
    currenths->distance, currenths->distance_calculation_method,
    currenths->MACAddress(), this->osscan_opentcpport,
    this->osscan_closedtcpport, this->osscan_closedudpport);
  result << str << str_92n_2_92n_56437;

  FPR = (FingerPrintResultsIPv6 *) currenths->FPR;
  assert(FPR->begin_time.tv_sec != 0);
  for (i = 0; i < sizeof(FPR->fp_responses) / sizeof(FPR->fp_responses[0]); i++) {
    const FPResponse *resp;
    std::string scrubbed;

    resp = this->fp_responses[i];
    if (resp == NULL)
      continue;
    scrubbed = get_scrubbed_buffer(resp);
    if (wrapit)
      scrubbed = run_length_encode(scrubbed);
    result << resp->probe_id << str_40P61_3_40P61_49531663 << scrubbed;
    assert(resp->senttime.tv_sec != 0);
    result << str_37ST6_4_7ST61_1513435456 << TIMEVAL_FSEC_SUBTRACT(resp->senttime, FPR->begin_time);
    assert(resp->rcvdtime.tv_sec != 0);
    result << str_37RT6_4_7RT61_1513405665 << TIMEVAL_FSEC_SUBTRACT(resp->rcvdtime, FPR->begin_time);
    result << str_4192n_3_4192n_49539288;
  }

  result << str_EXTRA_6_TRA40_587307412;
  result << str_FL61_3_FL61_2160129;
  result.fill('0');
  result << std::hex;
  result.width(5);
  result << FPR->flow_label;
  result << str_4192n_3_4192n_49539288;

  output = result.str();
  if (wrapit) {
    output = wrap(output);
  }

  Strncpy(str, output.c_str(), sizeof(str));

  return str;
}

static void write_merged_fpr(const FingerPrintResults *FPR,
                             const Target *currenths,
                             bool isGoodFP, bool wrapit) {
  log_write(LOG_NORMAL | LOG_SKID_NOXLT | LOG_STDOUT,
            str_TCP47_25_7s92n_803582942,
            FPR->merge_fpr(currenths, isGoodFP, wrapit));

  /* Added code here to print fingerprint to XML file any time it would be
     printed to any other output format  */
  xml_open_start_tag(str_osfin_13_print_1919286144);
  xml_attribute(str_finge_11_print_1375934236, str_37s_2_37s_50831, FPR->merge_fpr(currenths, isGoodFP, wrapit));
  xml_close_empty_tag();
  xml_newline();
}

/* Prints the formatted OS Scan output to stdout, logfiles, etc (but only
   if an OS Scan was performed).*/
void printosscanoutput(const Target *currenths) {
  int i;
  char numlst[512];             /* For creating lists of numbers */
  char *p;                      /* Used in manipulating numlst above */
  FingerPrintResults *FPR;
  int osscan_flag;

  if (!(osscan_flag = currenths->osscanPerformed()))
    return;

  if (currenths->FPR == NULL)
    return;
  FPR = currenths->FPR;

  xml_start_tag(str_os_2_os_3556);
  if (FPR->osscan_opentcpport > 0) {
    xml_open_start_tag(str_portu_8_tused_729373630);
    xml_attribute(str_state_5_state_109757585, str_open_4_open_3417674);
    xml_attribute(str_proto_5_proto_106940904, str_tcp_3_tcp_114657);
    xml_attribute(str_porti_6_ortid_982480548, str_37d_2_37d_50816, FPR->osscan_opentcpport);
    xml_close_empty_tag();
    xml_newline();
  }
  if (FPR->osscan_closedtcpport > 0) {
    xml_open_start_tag(str_portu_8_tused_729373630);
    xml_attribute(str_state_5_state_109757585, str_close_6_losed_1357520532);
    xml_attribute(str_proto_5_proto_106940904, str_tcp_3_tcp_114657);
    xml_attribute(str_porti_6_ortid_982480548, str_37d_2_37d_50816, FPR->osscan_closedtcpport);
    xml_close_empty_tag();
    xml_newline();
  }
  if (FPR->osscan_closedudpport > 0) {
    xml_open_start_tag(str_portu_8_tused_729373630);
    xml_attribute(str_state_5_state_109757585, str_close_6_losed_1357520532);
    xml_attribute(str_proto_5_proto_106940904, str_udp_3_udp_115649);
    xml_attribute(str_porti_6_ortid_982480548, str_37d_2_37d_50816, FPR->osscan_closedudpport);
    xml_close_empty_tag();
    xml_newline();
  }

  if (osscan_flag == OS_PERF_UNREL &&
      !(FPR->overall_results == OSSCAN_TOOMANYMATCHES ||
        (FPR->num_perfect_matches > 8 && !o.debugging)))
    log_write(LOG_PLAIN, str_Warni_103_rt92n_1345521281);

  // If the FP can't be submitted anyway, might as well make a guess.
  const char *reason = FPR->OmitSubmissionFP();
  printosclassificationoutput(FPR->getOSClassification(), o.osscan_guess || reason);

  if (FPR->overall_results == OSSCAN_SUCCESS &&
      (FPR->num_perfect_matches <= 8 || o.debugging)) {
    /* Success, not too many perfect matches. */
    if (FPR->num_perfect_matches > 0) {
      /* Some perfect matches. */
      for (i = 0; i < FPR->num_perfect_matches; i++)
        write_xml_osmatch(FPR->matches[i], FPR->accuracy[i]);

      log_write(LOG_MACHINE, str_92tOS_8_3237s_1103078030, FPR->matches[0]->OS_name);
      for (i = 1; i < FPR->num_perfect_matches; i++)
        log_write(LOG_MACHINE, str_12437_3_2437s_1450604412, FPR->matches[i]->OS_name);

      unsigned short numprints = FPR->matches[0]->numprints;
      log_write(LOG_PLAIN, str_OS32d_14_3237s_1203891342, FPR->matches[0]->OS_name);
      for (i = 1; i < FPR->num_perfect_matches; i++) {
        numprints = MIN(numprints, FPR->matches[i]->numprints);
        log_write(LOG_PLAIN, str_44323_4_3237s_442906960, FPR->matches[i]->OS_name);
      }
      log_write(LOG_PLAIN, str_92n_2_92n_56437);

      /* Suggest submission of an already-matching IPv6 fingerprint with
       * decreasing probability as numprints increases, and never if the group
       * has 5 or more prints or if the print is unsuitable. */
      bool suggest_submission = currenths->af() == AF_INET6 && reason == NULL && rand() % 5 >= numprints;
      if (suggest_submission)
        log_write(LOG_NORMAL | LOG_SKID_NOXLT | LOG_STDOUT,
            str_Nmap3_98_4792n_1862627678);
      if (suggest_submission || o.debugging || o.verbose > 1)
        write_merged_fpr(FPR, currenths, reason == NULL, true);
    } else {
      /* No perfect matches. */
      if ((o.verbose > 1 || o.debugging) && reason)
        log_write(LOG_NORMAL | LOG_SKID_NOXLT | LOG_STDOUT,
                  str_OS32f_38_7s92n_1138680940, reason);

      for (i = 0; i < 10 && i < FPR->num_matches && FPR->accuracy[i] > FPR->accuracy[0] - 0.10; i++)
        write_xml_osmatch(FPR->matches[i], FPR->accuracy[i]);

      if ((o.osscan_guess || reason) && FPR->num_matches > 0) {
        /* Print the best guesses available */
        log_write(LOG_PLAIN, str_Aggre_33_73741_847124572,
                  FPR->matches[0]->OS_name, floor(FPR->accuracy[0] * 100));
        for (i = 1; i < 10 && FPR->num_matches > i && FPR->accuracy[i] > FPR->accuracy[0] - 0.10; i++)
          log_write(LOG_PLAIN, str_44323_12_73741_2023186886, FPR->matches[i]->OS_name, floor(FPR->accuracy[i] * 100));

        log_write(LOG_PLAIN, str_92n_2_92n_56437);
      }

      if (!reason) {
        log_write(LOG_NORMAL | LOG_SKID_NOXLT | LOG_STDOUT,
                  str_No32e_101_4692n_834467284);
        write_merged_fpr(FPR, currenths, true, true);
      } else {
        log_write(LOG_NORMAL | LOG_SKID_NOXLT | LOG_STDOUT,
                  str_No32e_59_4692n_1246849773);
        if (o.verbose > 1 || o.debugging)
          write_merged_fpr(FPR, currenths, false, false);
      }
    }
  } else if (FPR->overall_results == OSSCAN_NOMATCHES) {
    /* No matches at all. */
    if (!reason) {
      log_write(LOG_NORMAL | LOG_SKID_NOXLT | LOG_STDOUT,
                str_No32O_95_4692n_542197518);
      write_merged_fpr(FPR, currenths, true, true);
    } else {
      log_write(LOG_NORMAL | LOG_SKID_NOXLT | LOG_STDOUT,
                str_OS32f_38_7s92n_1138680940, reason);
      log_write(LOG_NORMAL | LOG_SKID_NOXLT | LOG_STDOUT,
                str_No32O_24_st92n_525228972);
      if (o.debugging || o.verbose > 1)
        write_merged_fpr(FPR, currenths, false, false);
    }
  } else if (FPR->overall_results == OSSCAN_TOOMANYMATCHES
             || (FPR->num_perfect_matches > 8 && !o.debugging)) {
    /* Too many perfect matches. */
    log_write(LOG_NORMAL | LOG_SKID_NOXLT | LOG_STDOUT,
              str_Too32_67_ls92n_1280793254);
    if (o.debugging || o.verbose > 1)
      write_merged_fpr(FPR, currenths, false, false);
  } else {
    assert(0);
  }

  xml_end_tag(); /* os */
  xml_newline();

  if (currenths->seq.lastboot) {
    char tmbuf[128];
    struct timeval tv;
    double uptime;
    int err = n_ctime(tmbuf, sizeof(tmbuf), &currenths->seq.lastboot);
    chomp(tmbuf);
    gettimeofday(&tv, NULL);
    uptime = difftime(tv.tv_sec, currenths->seq.lastboot);
    if (o.verbose) {
      if (err)
        log_write(LOG_PLAIN, str_Uptim_25_ys92n_1724342356,
                uptime / 86400);
      else
        log_write(LOG_PLAIN, str_Uptim_36_4192n_48826662,
                uptime / 86400,
                tmbuf);
    }
    xml_open_start_tag(str_uptim_6_ptime_838362136);
    xml_attribute(str_secon_7_conds_1970096767, str_37460_4_7460f_1512482972, uptime);
    if (!err)
      xml_attribute(str_lastb_8_tboot_1458990456, str_37s_2_37s_50831, tmbuf);
    xml_close_empty_tag();
    xml_newline();
  }

  if (currenths->distance != -1) {
    log_write(LOG_PLAIN, str_Netwo_28_7s92n_1890723668,
              currenths->distance, (currenths->distance == 1) ? "" : str_s_1_s_115);
    xml_open_start_tag(str_dista_8_tance_288459765);
    xml_attribute(str_value_5_value_111972721, str_37d_2_37d_50816, currenths->distance);
    xml_close_empty_tag();
    xml_newline();
  }

  if (currenths->seq.responses > 3) {
    p = numlst;
    for (i = 0; i < currenths->seq.responses; i++) {
      if (p - numlst > (int) (sizeof(numlst) - 15))
        fatal(str_STRAN_57_rg92n_1726722643);
      if (p != numlst)
        *p++ = ',';
      sprintf(p, str_37X_2_37X_50804, currenths->seq.seqs[i]);
      while (*p)
        p++;
    }

    xml_open_start_tag(str_tcpse_11_uence_510183074);
    xml_attribute(str_index_5_index_100346066, str_37li_3_37li_1575649, (long) currenths->seq.index);
    xml_attribute(str_diffi_10_culty_1829500859, str_37s_2_37s_50831, seqidx2difficultystr(currenths->seq.index));
    xml_attribute(str_value_6_alues_823812830, str_37s_2_37s_50831, numlst);
    xml_close_empty_tag();
    xml_newline();
    if (o.verbose)
      log_write(LOG_PLAIN, str_TCP32_45_4192n_415563197, currenths->seq.index, seqidx2difficultystr(currenths->seq.index));

    log_write(LOG_MACHINE, str_92tSe_15_3237d_1408496847, currenths->seq.index);
  }

  if (currenths->seq.responses > 2) {
    p = numlst;
    for (i = 0; i < currenths->seq.responses; i++) {
      if (p - numlst > (int) (sizeof(numlst) - 15))
        fatal(str_STRAN_57_rg92n_1734482002);
      if (p != numlst)
        *p++ = ',';
      sprintf(p, str_37hX_3_37hX_1575508, currenths->seq.ipids[i]);
      while (*p)
        p++;
    }
    xml_open_start_tag(str_ipids_12_uence_1187626429);
    xml_attribute(str_class_5_class_94742904, str_37s_2_37s_50831, ipidclass2ascii(currenths->seq.ipid_seqclass));
    xml_attribute(str_value_6_alues_823812830, str_37s_2_37s_50831, numlst);
    xml_close_empty_tag();
    xml_newline();
    if (o.verbose)
      log_write(LOG_PLAIN, str_IP32I_31_7s92n_938311936,
                ipidclass2ascii(currenths->seq.ipid_seqclass));
    log_write(LOG_MACHINE, str_92tIP_15_3237s_1072092457,
              ipidclass2ascii(currenths->seq.ipid_seqclass));

    p = numlst;
    for (i = 0; i < currenths->seq.responses; i++) {
      if (p - numlst > (int) (sizeof(numlst) - 15))
        fatal(str_STRAN_57_rg92n_1718963284);
      if (p != numlst)
        *p++ = ',';
      sprintf(p, str_37X_2_37X_50804, currenths->seq.timestamps[i]);
      while (*p)
        p++;
    }

    xml_open_start_tag(str_tcpts_13_uence_845827615);
    xml_attribute(str_class_5_class_94742904, str_37s_2_37s_50831, tsseqclass2ascii(currenths->seq.ts_seqclass));
    if (currenths->seq.ts_seqclass != TS_SEQ_UNSUPPORTED) {
      xml_attribute(str_value_6_alues_823812830, str_37s_2_37s_50831, numlst);
    }
    xml_close_empty_tag();
    xml_newline();
  }
  log_flush_all();
}

/* An auxiliary function for printserviceinfooutput(). Returns
   non-zero if a and b are considered the same hostnames. */
static int hostcmp(const char *a, const char *b) {
  return strcasecmp(a, b) == 0;
}

/* Prints the alternate hostname/OS/device information we got from the service
   scan (if it was performed) */
void printserviceinfooutput(const Target *currenths) {
  Port *p = NULL;
  Port port;
  struct serviceDeductions sd;
  int i, numhostnames = 0, numostypes = 0, numdevicetypes = 0, numcpes = 0;
  char hostname_tbl[MAX_SERVICE_INFO_FIELDS][FQDN_LEN+1];
  char ostype_tbl[MAX_SERVICE_INFO_FIELDS][64];
  char devicetype_tbl[MAX_SERVICE_INFO_FIELDS][64];
  char cpe_tbl[MAX_SERVICE_INFO_FIELDS][80];
  const char *delim;

  for (i = 0; i < MAX_SERVICE_INFO_FIELDS; i++)
    hostname_tbl[i][0] = ostype_tbl[i][0] = devicetype_tbl[i][0] = cpe_tbl[i][0] = '\0';

  while ((p = currenths->ports.nextPort(p, &port, TCPANDUDPANDSCTP, PORT_OPEN))) {
    // The following 2 lines (from portlist.h) tell us that we don't need to
    // worry about free()ing anything in the serviceDeductions struct. pass in
    // an allocated struct serviceDeductions (don't worry about initializing, and
    // you don't have to free any internal ptrs.
    currenths->ports.getServiceDeductions(p->portno, p->proto, &sd);

    if (sd.hostname && !hostcmp(currenths->HostName(), sd.hostname)) {
      for (i = 0; i < MAX_SERVICE_INFO_FIELDS; i++) {
        if (hostname_tbl[i][0] && hostcmp(&hostname_tbl[i][0], sd.hostname))
          break;

        if (!hostname_tbl[i][0]) {
          numhostnames++;
          strncpy(&hostname_tbl[i][0], sd.hostname, sizeof(hostname_tbl[i]));
          break;
        }
      }
    }

    if (sd.ostype) {
      for (i = 0; i < MAX_SERVICE_INFO_FIELDS; i++) {
        if (ostype_tbl[i][0] && !strcmp(&ostype_tbl[i][0], sd.ostype))
          break;

        if (!ostype_tbl[i][0]) {
          numostypes++;
          strncpy(&ostype_tbl[i][0], sd.ostype, sizeof(ostype_tbl[i]));
          break;
        }
      }
    }

    if (sd.devicetype) {
      for (i = 0; i < MAX_SERVICE_INFO_FIELDS; i++) {
        if (devicetype_tbl[i][0] && !strcmp(&devicetype_tbl[i][0], sd.devicetype))
          break;

        if (!devicetype_tbl[i][0]) {
          numdevicetypes++;
          strncpy(&devicetype_tbl[i][0], sd.devicetype, sizeof(devicetype_tbl[i]));
          break;
        }
      }
    }

    for (std::vector<char *>::const_iterator it = sd.cpe.begin(); it != sd.cpe.end(); it++) {
      for (i = 0; i < MAX_SERVICE_INFO_FIELDS; i++) {
        if (cpe_tbl[i][0] && !strcmp(&cpe_tbl[i][0], *it))
          break;
        /* Applications (CPE part "a") aren't shown in this summary list in
           normal output. "a" classifications belong to an individual port, not
           the entire host, unlike "h" (hardware) and "o" (operating system).
           There isn't a good place to put the "a" classifications, so they are
           written to XML only. */
        if (cpe_get_part(*it) == 'a')
          break;

        if (!cpe_tbl[i][0]) {
          numcpes++;
          strncpy(&cpe_tbl[i][0], *it, sizeof(cpe_tbl[i]));
          break;
        }
      }
    }

  }

  if (!numhostnames && !numostypes && !numdevicetypes && !numcpes)
    return;

  log_write(LOG_PLAIN, str_Servi_13_nfo58_1816764251);

  delim = str_32_1_32_1631;
  if (numhostnames) {
    log_write(LOG_PLAIN, str_37sHo_12_3237s_1224331701, delim, numhostnames == 1 ? "" : str_s_1_s_115, &hostname_tbl[0][0]);
    for (i = 1; i < MAX_SERVICE_INFO_FIELDS; i++) {
      if (hostname_tbl[i][0])
        log_write(LOG_PLAIN, str_44323_4_3237s_442906960, &hostname_tbl[i][0]);
    }
    delim = str_5932_2_5932_1635331;
  }

  if (numostypes) {
    log_write(LOG_PLAIN, str_37sOS_10_3237s_1110653295, delim, numostypes == 1 ? "" : str_s_1_s_115,
              &ostype_tbl[0][0]);
    for (i = 1; i < MAX_SERVICE_INFO_FIELDS; i++) {
      if (ostype_tbl[i][0])
        log_write(LOG_PLAIN, str_44323_4_3237s_442906960, &ostype_tbl[i][0]);
    }
    delim = str_5932_2_5932_1635331;
  }

  if (numdevicetypes) {
    log_write(LOG_PLAIN, str_37sDe_14_3237s_60155613, delim,
              numdevicetypes == 1 ? "" : str_s_1_s_115, &devicetype_tbl[0][0]);
    for (i = 1; i < MAX_SERVICE_INFO_FIELDS; i++) {
      if (devicetype_tbl[i][0])
        log_write(LOG_PLAIN, str_44323_4_3237s_442906960, &devicetype_tbl[i][0]);
    }
    delim = str_5932_2_5932_1635331;
  }

  if (numcpes > 0) {
    log_write(LOG_PLAIN, str_37sCP_9_3237s_1469228444, delim, &cpe_tbl[0][0]);
    for (i = 1; i < MAX_SERVICE_INFO_FIELDS; i++) {
      if (cpe_tbl[i][0])
        log_write(LOG_PLAIN, str_44323_4_3237s_442906960, &cpe_tbl[i][0]);
    }
    delim = str_5932_2_5932_1635331;
  }

  log_write(LOG_PLAIN, str_92n_2_92n_56437);
  log_flush_all();
}

#ifndef NOLUA
void printscriptresults(const ScriptResults *scriptResults, stype scantype) {
  ScriptResults::const_iterator iter;
  char *script_output;

  if (scriptResults->size() > 0) {
    if (scantype == SCRIPT_PRE_SCAN) {
      xml_start_tag(str_presc_9_cript_923681490);
      log_write(LOG_PLAIN, str_Pre45_26_5892n_309602042);
    } else {
      xml_start_tag(str_posts_10_cript_1775406741);
      log_write(LOG_PLAIN, str_Post4_27_5892n_1898651075);
    }
    for (iter = scriptResults->begin(); iter != scriptResults->end(); iter++) {
      (*iter)->write_xml();
      script_output = formatScriptOutput((*iter));
      if (script_output != NULL) {
        log_write(LOG_PLAIN, str_37s92_4_7s92n_1514362758, script_output);
        free(script_output);
      }
    }
    xml_end_tag();
  }
}

void printhostscriptresults(const Target *currenths) {
  ScriptResults::const_iterator iter;
  char *script_output;

  if (currenths->scriptResults.size() > 0) {
    xml_start_tag(str_hosts_10_cript_203298701);
    log_write(LOG_PLAIN, str_92nHo_24_5892n_933267202);
    for (iter = currenths->scriptResults.begin();
         iter != currenths->scriptResults.end();
         iter++) {
      (*iter)->write_xml();

      script_output = formatScriptOutput((*iter));
      if (script_output != NULL) {
        log_write(LOG_PLAIN, str_37s92_4_7s92n_1514362758, script_output);
        free(script_output);
      }
    }
    xml_end_tag();
  }
}
#endif

/* Print a table with traceroute hops. */
static void printtraceroute_normal(const Target *currenths) {
  static const int HOP_COL = 0, RTT_COL = 1, HOST_COL = 2;
  NmapOutputTable Tbl(currenths->traceroute_hops.size() + 1, 3);
  struct probespec probe;
  std::list<TracerouteHop>::const_iterator it;
  int row;

  /* No trace, must be localhost. */
  if (currenths->traceroute_hops.size() == 0)
    return;

  /* Print header. */
  log_write(LOG_PLAIN, "\n");
  probe = currenths->traceroute_probespec;
  if (probe.type == PS_TCP) {
    log_write(LOG_PLAIN, str_TRACE_31_4192n_1196774088,
              probe.pd.tcp.dport, proto2ascii_lowercase(probe.proto));
  } else if (probe.type == PS_UDP) {
    log_write(LOG_PLAIN, str_TRACE_31_4192n_1196774088,
              probe.pd.udp.dport, proto2ascii_lowercase(probe.proto));
  } else if (probe.type == PS_SCTP) {
    log_write(LOG_PLAIN, str_TRACE_31_4192n_1196774088,
              probe.pd.sctp.dport, proto2ascii_lowercase(probe.proto));
  } else if (probe.type == PS_ICMP || probe.type == PS_ICMPV6 || probe.type == PS_PROTO) {
    const struct nprotoent *proto = nmap_getprotbynum(probe.proto);
    log_write(LOG_PLAIN, str_TRACE_32_4192n_976595319,
              probe.proto, proto ? proto->p_name : "unknown");
  } else if (probe.type == PS_NONE) {
    /* "Traces" of directly connected targets don't send any packets. */
    log_write(LOG_PLAIN, str_TRACE_12_TE92n_1667659311);
  } else {
    fatal(str_Unkno_22_37d46_1253299229, probe.type);
  }

  row = 0;
  Tbl.addItem(row, HOP_COL, false, str_HOP_3_HOP_71721);
  Tbl.addItem(row, RTT_COL, false, str_RTT_3_RTT_81490);
  Tbl.addItem(row, HOST_COL, false, str_ADDRE_7_DRESS_429709356);
  row++;

  it = currenths->traceroute_hops.begin();

  if (!o.debugging) {
    /* Consolidate shared hops. */
    const TracerouteHop *shared_hop = NULL;
    const struct sockaddr_storage *addr = currenths->TargetSockAddr();
    while (it != currenths->traceroute_hops.end()
           && !sockaddr_storage_equal(&it->tag, addr)) {
      shared_hop = &*it;
      it++;
    }

    if (shared_hop != NULL) {
      Tbl.addItem(row, HOP_COL, false, str_45_1_45_1665);
      if (shared_hop->ttl == 1) {
        Tbl.addItemFormatted(row, RTT_COL, true,
          str_Hop32_27_3237s_30615772,
          inet_ntop_ez(&shared_hop->tag, sizeof(shared_hop->tag)));
      } else if (shared_hop->ttl > 1) {
        Tbl.addItemFormatted(row, RTT_COL, true,
          str_Hops3_32_3237s_1118518768, shared_hop->ttl,
          inet_ntop_ez(&shared_hop->tag, sizeof(shared_hop->tag)));
      }
      row++;
    }
  }

  while (it != currenths->traceroute_hops.end()) {
    Tbl.addItemFormatted(row, HOP_COL, false, str_37d_2_37d_50816, it->ttl);
    if (it->timedout) {
      if (o.debugging) {
        Tbl.addItem(row, RTT_COL, false, str_46464_3_64646_1540188678);
        it++;
      } else {
        /* The beginning and end of timeout consolidation. */
        int begin_ttl, end_ttl;
        begin_ttl = end_ttl = it->ttl;
        for (; it != currenths->traceroute_hops.end() && it->timedout; it++)
          end_ttl = it->ttl;
        if (begin_ttl == end_ttl)
          Tbl.addItem(row, RTT_COL, false, str_46464_3_64646_1540188678);
        else
          Tbl.addItemFormatted(row, RTT_COL, false, str_46464_6_3237d_349693573, end_ttl);
      }
      row++;
    } else {
      /* Normal hop output. */
      char namebuf[256];

      it->display_name(namebuf, sizeof(namebuf));
      if (it->rtt < 0)
        Tbl.addItem(row, RTT_COL, false, str_4545_2_4545_1601730);
      else
        Tbl.addItemFormatted(row, RTT_COL, false, str_37462_7_f32ms_581608479, it->rtt);
      Tbl.addItemFormatted(row, HOST_COL, false, str_37s_2_37s_50831, namebuf);
      row++;
      it++;
    }
  }

  log_write(LOG_PLAIN, str_37s_2_37s_50831, Tbl.printableTable(NULL));

  log_flush(LOG_PLAIN);
}

static void printtraceroute_xml(const Target *currenths) {
  struct probespec probe;
  std::list<TracerouteHop>::const_iterator it;

  /* No trace, must be localhost. */
  if (currenths->traceroute_hops.size() == 0)
    return;

  /* XML traceroute header */
  xml_open_start_tag(str_trace_5_trace_110620997);

  probe = currenths->traceroute_probespec;
  if (probe.type == PS_TCP) {
    xml_attribute(str_port_4_port_3446913, str_37d_2_37d_50816, probe.pd.tcp.dport);
    xml_attribute(str_proto_5_proto_106940904, str_37s_2_37s_50831, proto2ascii_lowercase(probe.proto));
  } else if (probe.type == PS_UDP) {
    xml_attribute(str_port_4_port_3446913, str_37d_2_37d_50816, probe.pd.udp.dport);
    xml_attribute(str_proto_5_proto_106940904, str_37s_2_37s_50831, proto2ascii_lowercase(probe.proto));
  } else if (probe.type == PS_SCTP) {
    xml_attribute(str_port_4_port_3446913, str_37d_2_37d_50816, probe.pd.sctp.dport);
    xml_attribute(str_proto_5_proto_106940904, str_37s_2_37s_50831, proto2ascii_lowercase(probe.proto));
  } else if (probe.type == PS_ICMP || probe.type == PS_PROTO) {
    const struct nprotoent *proto = nmap_getprotbynum(probe.proto);
    if (proto == NULL)
      xml_attribute(str_proto_5_proto_106940904, str_37d_2_37d_50816, probe.proto);
    else
      xml_attribute(str_proto_5_proto_106940904, str_37s_2_37s_50831, proto->p_name);
  }
  xml_close_start_tag();
  xml_newline();

  for (it = currenths->traceroute_hops.begin();
       it != currenths->traceroute_hops.end();
       it++) {
    if (it->timedout)
      continue;
    xml_open_start_tag(str_hop_3_hop_103497);
    xml_attribute(str_ttl_3_ttl_115180, str_37d_2_37d_50816, it->ttl);
    xml_attribute(str_ipadd_6_paddr_1182483048, str_37s_2_37s_50831, inet_ntop_ez(&it->addr, sizeof(it->addr)));
    if (it->rtt < 0)
      xml_attribute(str_rtt_3_rtt_113266, str_4545_2_4545_1601730);
    else
      xml_attribute(str_rtt_3_rtt_113266, str_37462_4_7462f_1512483034, it->rtt);
    if (!it->name.empty())
      xml_attribute(str_host_4_host_3208616, str_37s_2_37s_50831, it->name.c_str());
    xml_close_empty_tag();
    xml_newline();
  }

  /* traceroute XML footer */
  xml_end_tag();
  xml_newline();
  log_flush(LOG_XML);
}

void printtraceroute(const Target *currenths) {
  printtraceroute_normal(currenths);
  printtraceroute_xml(currenths);
}

void printtimes(const Target *currenths) {
  if (currenths->to.srtt != -1 || currenths->to.rttvar != -1) {
    if (o.debugging) {
      log_write(LOG_STDOUT, str_Final_51_7d92n_2132347005,
        currenths->to.srtt, currenths->to.rttvar, currenths->to.timeout);
    }
    xml_open_start_tag(str_times_5_times_110364486);
    xml_attribute(str_srtt_4_srtt_3539231, str_37d_2_37d_50816, currenths->to.srtt);
    xml_attribute(str_rttva_6_ttvar_920543371, str_37d_2_37d_50816, currenths->to.rttvar);
    xml_attribute(str_to_2_to_3707, str_37d_2_37d_50816, currenths->to.timeout);
    xml_close_empty_tag();
    xml_newline();
  }
}

/* Prints a status message while the program is running */
void printStatusMessage() {
  // Pre-computations
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int time = (int) (o.TimeSinceStart(&tv));

  log_write(LOG_STDOUT, str_Stats_75_7s92n_1189817354,
            time / 60 / 60, time / 60 % 60, time % 60, o.numhosts_scanned,
            o.numhosts_up, o.numhosts_scanning,
            scantype2str(o.current_scantype));
}

/* Prints the beginning of a str_finis_8_ished_673660814 start tag, with time, timestr, and
   elapsed attributes. Leaves the start tag open so you can add more attributes.
   You have to close the tag with xml_close_empty_tag. */
void print_xml_finished_open(time_t timep, const struct timeval *tv) {
  char mytime[128];
  int err = n_ctime(mytime, sizeof(mytime), &timep);

  chomp(mytime);

  xml_open_start_tag("finished");
  xml_attribute(str_time_4_time_3560141, str_37lu_3_37lu_1575661, (unsigned long) timep);
  if (!err) {
    xml_attribute(str_times_7_mestr_1313907644, str_37s_2_37s_50831, mytime);
    xml_attribute(str_summa_7_mmary_1857640538,
        str_Nmap3_57_conds_695852372,
        mytime, o.numhosts_scanned,
        (o.numhosts_scanned == 1) ? str_IP32a_10_dress_1532940142 : str_IP32a_12_esses_18302820,
        o.numhosts_up, (o.numhosts_up == 1) ? str_host_4_host_3208616 : str_hosts_5_hosts_99467211,
        o.TimeSinceStart(tv));
  }
  else {
    xml_attribute(str_summa_7_mmary_1857640538,
        str_Nmap3_51_conds_471326868,
        o.numhosts_scanned,
        (o.numhosts_scanned == 1) ? str_IP32a_10_dress_1532940142 : str_IP32a_12_esses_18302820,
        o.numhosts_up, (o.numhosts_up == 1) ? str_host_4_host_3208616 : str_hosts_5_hosts_99467211,
        o.TimeSinceStart(tv));
  }
  xml_attribute(str_elaps_7_apsed_1666428548, str_37462_4_7462f_1512483034, o.TimeSinceStart(tv));
}

void print_xml_hosts() {
  xml_open_start_tag(str_hosts_5_hosts_99467211);
  xml_attribute("up", str_37u_2_37u_50833, o.numhosts_up);
  xml_attribute("down", str_37u_2_37u_50833, o.numhosts_scanned - o.numhosts_up);
  xml_attribute(str_total_5_total_110549828, str_37u_2_37u_50833, o.numhosts_scanned);
  xml_close_empty_tag();
}

/* Prints the statistics and other information that goes at the very end
   of an Nmap run */
void printfinaloutput() {
  time_t timep;
  char mytime[128];
  int err = 0;
  struct timeval tv;
  char statbuf[128];

  gettimeofday(&tv, NULL);
  timep = time(NULL);

  if (o.numhosts_scanned == 0
#ifndef NOLUA
      && !o.scriptupdatedb
#endif
      )
    error(str_WARNI_55_ned46_170333584);
  if (o.numhosts_scanned == 1 && o.numhosts_up == 0 && !o.listscan &&
      o.pingtype != PINGTYPE_NONE)
    log_write(LOG_STDOUT, str_Note5_82_Pn92n_1781633607);
  else if (o.numhosts_up > 0) {
    if (o.osscan && o.servicescan)
      log_write(LOG_PLAIN, str_OS32a_103_4692n_1195790526);
    else if (o.osscan)
      log_write(LOG_PLAIN, str_OS32d_91_4692n_1775481852);
    else if (o.servicescan)
      log_write(LOG_PLAIN, str_Servi_96_4692n_498134219);
    else if (o.udpscan && o.defeat_icmp_ratelimit)
      log_write(LOG_PLAIN, str_WARNI_130_4692n_1335875190);

  }

  log_write(LOG_STDOUT | LOG_SKID,
            str_Nmap3_53_ds92n_1436429374,
            o.numhosts_scanned,
            (o.numhosts_scanned == 1) ? str_IP32a_10_dress_1532940142 : str_IP32a_12_esses_18302820,
            o.numhosts_up, (o.numhosts_up == 1) ? str_host_4_host_3208616 : str_hosts_5_hosts_99467211,
            o.TimeSinceStart(&tv));
  if (o.verbose && o.isr00t && o.RawScan())
    log_write(LOG_STDOUT | LOG_SKID, str_32323_15_7s92n_978281371,
              getFinalPacketStats(statbuf, sizeof(statbuf)));

  xml_start_tag(str_runst_8_stats_834595060);
  print_xml_finished_open(timep, &tv);
  xml_attribute(str_exit_4_exit_3127582, str_succe_7_ccess_1867169789);
  xml_close_empty_tag();
  print_xml_hosts();
  xml_newline();
  xml_end_tag();
  xml_newline();

  err = n_ctime(mytime, sizeof(mytime), &timep);
  if (!err) {
    chomp(mytime);
    log_write(LOG_NORMAL | LOG_MACHINE,
        str_3532N_63_ds92n_2145817541,
        mytime, o.numhosts_scanned,
        (o.numhosts_scanned == 1) ? str_IP32a_10_dress_1532940142 : str_IP32a_12_esses_18302820,
        o.numhosts_up, (o.numhosts_up == 1) ? str_host_4_host_3208616 : str_hosts_5_hosts_99467211,
        o.TimeSinceStart(&tv));
  }
  else {
    log_write(LOG_NORMAL | LOG_MACHINE,
        str_3532N_57_ds92n_70469731,
        o.numhosts_scanned,
        (o.numhosts_scanned == 1) ? str_IP32a_10_dress_1532940142 : str_IP32a_12_esses_18302820,
        o.numhosts_up, (o.numhosts_up == 1) ? str_host_4_host_3208616 : str_hosts_5_hosts_99467211,
        o.TimeSinceStart(&tv));
  }

  xml_end_tag(); /* nmaprun */
  xml_newline();
  log_flush_all();
}

/* A record consisting of a data file name ("nmap-services", "nmap-os-db",
   etc.), and the directory and file in which is was found. This is a
   broken-down version of what is stored in o.loaded_data_files. It is used in
   printdatafilepaths. */
struct data_file_record {
  std::string data_file;
  std::string dir;
  std::string file;

  /* Compares this record to another. First compare the directory names, then
     compare the file names. */
  bool operator<(const struct data_file_record &other) const {
    int cmp;

    cmp = dir.compare(other.dir);
    if (cmp == 0)
      cmp = file.compare(other.file);

    return cmp < 0;
  }
};

/* Prints the names of data files that were loaded and the paths at which they
   were found. */
void printdatafilepaths() {
  std::list<struct data_file_record> df;
  std::list<struct data_file_record>::const_iterator iter;
  std::map<std::string, std::string>::const_iterator map_iter;
  std::string dir;
  unsigned int num_dirs;

  /* Copy the elements of o.loaded_data_files (each a (data file, path) pair) to
     a list of data_file_records to make them easier to manipulate. */
  for (map_iter = o.loaded_data_files.begin();
       map_iter != o.loaded_data_files.end(); map_iter++) {
    struct data_file_record r;
    char *s;

    r.data_file = map_iter->first;
    s = path_get_dirname(map_iter->second.c_str());
    if (s == NULL)
      fatal(str_37s58_39_emory_1035032814, __func__);
    r.dir = std::string(s);
    free(s);
    s = path_get_basename(map_iter->second.c_str());
    if (s == NULL)
      fatal(str_37s58_39_emory_1035032814, __func__);
    r.file = std::string(s);
    free(s);

    df.push_back(r);
  }

  /* Sort the list, first by directory name, then by file name. This ensures
     that records with the same directory name are contiguous. */
  df.sort();

  /* Count the number of distinct directories. Normally we print something only
     if files came from more than one directory. */
  if (df.empty()) {
    num_dirs = 0;
  } else {
    num_dirs = 1;
    iter = df.begin();
    dir = iter->dir;
    for (iter++; iter != df.end(); iter++) {
      if (iter->dir != dir) {
        num_dirs++;
        dir = iter->dir;
      }
    }
  }

  /* Decide what to print out based on the number of distinct directories and
     the verbosity and debugging levels. */
  if (num_dirs == 0) {
    /* If no files were read, print a message only in debugging mode. */
    if (o.debugging > 0)
      log_write(LOG_PLAIN, str_No32d_21_4692n_1622645264);
  } else if (num_dirs == 1 && o.verbose && !o.debugging) {
    /* If all the files were from the same directory and we're in verbose mode,
       print a brief message unless we are also in debugging mode. */
    log_write(LOG_PLAIN, str_Read3_26_7s92n_1768018538, dir.c_str());
  } else if ((num_dirs == 1 && o.debugging) || num_dirs > 1) {
    /* If files were read from more than one directory, or if they were read
       from one directory and we are in debugging mode, display all the files
       grouped by directory. */
    iter = df.begin();
    while (iter != df.end()) {
      dir = iter->dir;
      /* Write the directory name. */
      log_write(LOG_PLAIN, str_Read3_13_37s58_1560176172, dir.c_str());
      /* Write files in that directory on the same line. */
      while (iter != df.end() && iter->dir == dir) {
        log_write(LOG_PLAIN, str_3237s_3_3237s_48639952, iter->file.c_str());
        iter++;
      }
      log_write(LOG_PLAIN, str_4692n_3_4692n_49688243);
    }
  }
}

static inline const char *nslog2str(nsock_loglevel_t loglevel) {
  switch(loglevel) {
    case NSOCK_LOG_DBG_ALL:
      return str_DEBUG_10_2FULL_565626209;
    case NSOCK_LOG_DBG:
      return str_DEBUG_5_DEBUG_64921139;
    case NSOCK_LOG_INFO:
      return str_INFO_4_INFO_2251950;
    case NSOCK_LOG_ERROR:
      return str_ERROR_5_ERROR_66247144;
    default:
      return str_63636_3_36363_1594733175;
  };
}

void nmap_adjust_loglevel(bool trace) {
  nsock_loglevel_t nsock_loglevel;

  if (o.debugging >= 7)
    nsock_loglevel = NSOCK_LOG_DBG_ALL;
  else if (o.debugging >= 4)
    nsock_loglevel = NSOCK_LOG_DBG;
  else if (trace || o.debugging >= 2)
    nsock_loglevel = NSOCK_LOG_INFO;
  else
    nsock_loglevel = NSOCK_LOG_ERROR;

  nsock_set_loglevel(nsock_loglevel);
}

static void nmap_nsock_stderr_logger(const struct nsock_log_rec *rec) {
  int elapsed_time;

  elapsed_time = TIMEVAL_MSEC_SUBTRACT(rec->time, *(o.getStartTime()));

  log_write(LOG_STDERR, str_NSOCK_27_7s92n_490926287, nslog2str(rec->level),
            elapsed_time/1000.0, rec->func, rec->msg);
}

void nmap_set_nsock_logger() {
  nsock_set_log_function(nmap_nsock_stderr_logger);
}
