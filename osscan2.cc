#include "./multilang/zh_CN/osscan2.h"

/***************************************************************************
 * osscan2.cc -- Routines used for 2nd Generation OS detection via         *
 * TCP/IP fingerprinting.  * For more information on how this works in     *
 * Nmap, see https://nmap.org/osdetect/                                     *
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

#include "osscan.h"
#include "osscan2.h"
#include "timing.h"
#include "NmapOps.h"
#include "tcpip.h"
#include "Target.h"
#include "utils.h"
#include "nmap_error.h"
#include "FPEngine.h"
#include "FingerPrintResults.h"
#include <dnet.h>

#include "struct_ip.h"

#include <list>
#include <math.h>

extern NmapOps o;
#ifdef WIN32
/* from libdnet's intf-win32.c */
extern str_C_1_C_67 int g_has_npcap_loopback;
#endif

/* 8 options:
 *  0~5: six options for SEQ/OPS/WIN/T1 probes.
 *  6:   ECN probe.
 *  7-12:   T2~T7 probes.
 *
 * option 0: WScale (10), Nop, MSS (1460), Timestamp, SackP
 * option 1: MSS (1400), WScale (0), SackP, T(0xFFFFFFFF,0x0), EOL
 * option 2: T(0xFFFFFFFF, 0x0), Nop, Nop, WScale (5), Nop, MSS (640)
 * option 3: SackP, T(0xFFFFFFFF,0x0), WScale (10), EOL
 * option 4: MSS (536), SackP, T(0xFFFFFFFF,0x0), WScale (10), EOL
 * option 5: MSS (265), SackP, T(0xFFFFFFFF,0x0)
 * option 6: WScale (10), Nop, MSS (1460), SackP, Nop, Nop
 * option 7-11: WScale (10), Nop, MSS (265), T(0xFFFFFFFF,0x0), SackP
 * option 12: WScale (15), Nop, MSS (265), T(0xFFFFFFFF,0x0), SackP
 */
static struct {
  u8* val;
  int len;
} prbOpts[] = {
  {(u8*) str_92x03_80_92x02_1103927526, 20},
  {(u8*) str_92x02_80_92x00_482715575, 20},
  {(u8*) str_92x08_80_92x80_1178316317, 20},
  {(u8*) str_92x04_64_92x00_1579719434, 16},
  {(u8*) str_92x02_80_92x00_612974045, 20},
  {(u8*) str_92x02_64_92x00_1854369393, 16},
  {(u8*) str_92x03_48_92x01_1146849469, 12},
  {(u8*) str_92x03_80_92x02_1227410913, 20},
  {(u8*) str_92x03_80_92x02_1227410913, 20},
  {(u8*) str_92x03_80_92x02_1227410913, 20},
  {(u8*) str_92x03_80_92x02_1227410913, 20},
  {(u8*) str_92x03_80_92x02_1227410913, 20},
  {(u8*) str_92x03_80_92x02_1500999388, 20}
};

/* TCP Window sizes. Numbering is the same as for prbOpts[] */
u16 prbWindowSz[] = { 1, 63, 4, 4, 16, 512, 3, 128, 256, 1024, 31337, 32768, 65535 };

/* Current time. It is globally accessible so it can save calls to gettimeofday() */
static struct timeval now;

/* Global to store performance info */
static struct scan_performance_vars perf;


/******************************************************************************
 * Miscellaneous functions                                                    *
 ******************************************************************************/

/* Return a value based on the IP ID sequence generation
   class (one of the IPID_SEQ_* constants). If ipid_seqclass is such that the
   test result should be omitted, the function returns NULL. */
static const char *make_aval_ipid_seq(int ipid_seqclass, u32 ipids[NUM_SEQ_SAMPLES],
                                       HostOsScanStats *hss) {
  switch (ipid_seqclass) {
  case IPID_SEQ_CONSTANT:
    return hss->target->FPR->cp_hex(ipids[0]);
    break;
  case IPID_SEQ_INCR_BY_2:
  case IPID_SEQ_INCR:
    return str_I_1_I_73;
    break;
  case IPID_SEQ_BROKEN_INCR:
    return str_BI_2_BI_2119;
    break;
  case IPID_SEQ_RPI:
    return str_RI_2_RI_2615;
    break;
  case IPID_SEQ_RD:
    return str_RD_2_RD_2610;
    break;
  case IPID_SEQ_ZERO:
    return str_Z_1_Z_90;
    break;
  default:
    /* Signal to omit test result. */
    return NULL;
    break;
  }
}


/* Returns a guess about the original TTL based on an observed TTL value.
 * This function assumes that the target from which we received the packet was
 * less than 32 hops away. Also, note that although some systems use an
 * initial TTL of 60, this function rounds that to 64, as both values
 * cannot be reliably distinguished based on a simple observed hop count. */
int get_initial_ttl_guess(u8 ttl) {
  if (ttl <= 32)
    return 32;
  else if (ttl <= 64)
    return 64;
  else if (ttl <= 128)
    return 128;
  else
    return 255;
}


/* This function takes an array of "numSamples" IP IDs and analyzes
 them to determine their sequence classification.  It returns
 one of the IPID_SEQ_* classifications defined in nmap.h .  If the
 function cannot determine the sequence, IPID_SEQ_UNKNOWN is returned.
 This islocalhost argument is a boolean specifying whether these
 numbers were generated by scanning localhost. */
int identify_sequence(int numSamples, u32 *ipid_diffs, int islocalhost) {
  int i, j, k, l;

  if (islocalhost) {
    int allgto = 1; /* ALL diffs greater than one */

    for (i = 0; i < numSamples - 1; i++) {
      if (ipid_diffs[i] < 2) {
        allgto = 0; break;
      }
    }

    if (allgto) {
      for (i = 0; i < numSamples - 1; i++) {
        if (ipid_diffs[i] % 256 == 0) /* Stupid MS */
          ipid_diffs[i] -= 256;
        else
          ipid_diffs[i]--; /* Because on localhost the RST sent back use an IPID */
      }
    }
  }

  /* Constant */
  j = 1; /* j is a flag meaning "all differences seen are zero" */
  for (i = 0; i < numSamples - 1; i++) {
    if (ipid_diffs[i] != 0) {
        j = 0;
    break;
    }
  }
  if (j) {
    return IPID_SEQ_CONSTANT;
  }

  /* Random Positive Increments */
  for (i = 0; i < numSamples - 1; i++) {
    if (ipid_diffs[i] > 1000 &&
        (ipid_diffs[i] % 256 != 0 ||
        (ipid_diffs[i] % 256 == 0 && ipid_diffs[i] >= 25600))) {
      return IPID_SEQ_RPI;
      }
    }

  j = 1; /* j is a flag meaning "all differences seen are < 10" */
  k = 1; /* k is a flag meaning "all difference seen are multiples of 256 and
          * no greater than 5120" */
  l = 1; /* l is a flag meaning "all differences are multiples of 2" */
  for (i = 0; i < numSamples - 1; i++) {
    if (k && (ipid_diffs[i] > 5120 || ipid_diffs[i] % 256 != 0)) {
      k = 0;
    }

    if (l && ipid_diffs[i] % 2 != 0) {
      l = 0;
    }

    if (j && ipid_diffs[i] > 9) {
      j = 0;
    }
  }

  /* Broken Increment */
  if (k == 1) {
    return IPID_SEQ_BROKEN_INCR;
  }

  /* Incrementing by 2 */
  if (l == 1)
    return IPID_SEQ_INCR_BY_2;

  /* Incremental by 1 */
  if (j == 1)
    return IPID_SEQ_INCR;

  return IPID_SEQ_UNKNOWN;
}

/* Calculate the distances between the ipids and write them
   into the ipid_diffs array. If the sequence class can be determined
   immediately, return it; otherwise return -1 */
int get_diffs(u32 *ipid_diffs, int numSamples, const u32 *ipids, int islocalhost) {
  int i;
  bool allipideqz = true;

  if (numSamples < 2)
    return IPID_SEQ_UNKNOWN;

  for (i = 1; i < numSamples; i++) {
    if (ipids[i - 1] != 0 || ipids[i] != 0)
      allipideqz = false; /* All IP.ID values do *NOT* equal zero */

    ipid_diffs[i - 1] = ipids[i] - ipids[i - 1];

    /* Random */
    if (numSamples > 2 && ipid_diffs[i - 1] > 20000)
      return IPID_SEQ_RD;
  }

  if (allipideqz) {
    return IPID_SEQ_ZERO;
  }
  else {
    return -1;
  }

}

/* Indentify the ipid sequence for 32-bit IPID values (IPv6) */
int get_ipid_sequence_32(int numSamples, const u32 *ipids, int islocalhost) {
  int ipid_seq = IPID_SEQ_UNKNOWN;
  u32 ipid_diffs[32];
  assert(numSamples < (int) (sizeof(ipid_diffs) / 2));
  ipid_seq = get_diffs(ipid_diffs, numSamples, ipids, islocalhost);
  if (ipid_seq < 0) {
    return identify_sequence(numSamples, ipid_diffs, islocalhost);
  }
  else {
    return ipid_seq;
  }
}

/* Indentify the ipid sequence for 16-bit IPID values (IPv4) */
int get_ipid_sequence_16(int numSamples, const u32 *ipids, int islocalhost) {
  int i;
  int ipid_seq = IPID_SEQ_UNKNOWN;
  u32 ipid_diffs[32];
  assert(numSamples < (int) (sizeof(ipid_diffs) / 2));
  ipid_seq = get_diffs(ipid_diffs, numSamples, ipids, islocalhost);
  /* AND with 0xffff so that in case the 16 bit counter was
   * flipped over we still have a continuous sequence */
  for (i = 0; i < numSamples; i++) {
    ipid_diffs[i] = ipid_diffs[i] & 0xffff;
  }
  if (ipid_seq < 0) {
    return identify_sequence(numSamples, ipid_diffs, islocalhost);
  }
  else {
    return ipid_seq;
  }
}

/* Convert a TCP sequence prediction difficulty index like 1264386
   into a difficulty string like "Worthy Challenge */
const char *seqidx2difficultystr(unsigned long idx) {
  return  (idx < 3) ? str_Trivi_12_2joke_859791271 : (idx < 6) ? str_Easy_4_Easy_2152482 : (idx < 11) ? str_Mediu_6_edium_1994163307 : (idx < 12) ? str_Formi_10_dable_1763056935 : (idx < 16) ? str_Worth_16_lenge_807251399 : str_Good3_10_uck33_1304429773;
}

const char *ipidclass2ascii(int seqclass) {
  switch (seqclass) {
  case IPID_SEQ_CONSTANT:
    return str_Dupli_19_03341_247225330;
  case IPID_SEQ_INCR:
    return str_Incre_11_ental_1541726278;
  case IPID_SEQ_INCR_BY_2:
    return str_Incre_17_by322_679535254;
  case IPID_SEQ_BROKEN_INCR:
    return str_Broke_32_ental_239637131;
  case IPID_SEQ_RD:
    return str_Rando_10_mized_2109749491;
  case IPID_SEQ_RPI:
    return str_Rando_26_ments_2032467842;
  case IPID_SEQ_ZERO:
    return str_All32_9_zeros_1285189419;
  case IPID_SEQ_UNKNOWN:
    return str_Busy3_28_class_58654837;
  default:
    return str_ERROR_11_WTF63_193252127;
  }
}

const char *tsseqclass2ascii(int seqclass) {
  switch (seqclass) {
  case TS_SEQ_ZERO:
    return str_zero3_14_stamp_559831569;
  case TS_SEQ_2HZ:
    return str_2HZ_3_2HZ_50372;
  case TS_SEQ_100HZ:
    return str_100HZ_5_100HZ_46730947;
  case TS_SEQ_1000HZ:
    return str_1000H_6_000HZ_1448635825;
  case TS_SEQ_OTHER_NUM:
    return str_other_5_other_106069776;
  case TS_SEQ_UNSUPPORTED:
    return str_none3_27_ted41_499172207;
  case TS_SEQ_UNKNOWN:
    return str_unkno_13_class_1198363375;
  default:
    return str_ERROR_11_WTF63_193252127;
  }
}


/** Sets up the pcap descriptor in HOS (obtains a descriptor and sets the
 * appropriate BPF filter, based on the supplied list of targets). */
static void begin_sniffer(HostOsScan *HOS, std::vector<Target *> &Targets) {
  char pcap_filter[2048];
  /* 20 IPv6 addresses is max (45 byte addy + 14 (" or src host ")) * 20 == 1180 */
  char dst_hosts[1200];
  int filterlen = 0;
  int len;
  unsigned int targetno;
  bool doIndividual = Targets.size() <= 20; // Don't bother IP limits if scanning huge # of hosts
  pcap_filter[0] = '\0';

  /* If we have 20 or less targets, build a list of addresses so we can set
   * an explicit BPF filter */
  if (doIndividual) {
    for (targetno = 0; targetno < Targets.size(); targetno++) {
      len = Snprintf(dst_hosts + filterlen,
                     sizeof(dst_hosts) - filterlen,
                     str_37ssr_13_3237s_1089977452, (targetno == 0)? "" : str_32or3_4_2or32_1509680737,
                     Targets[targetno]->targetipstr());
      if (len < 0 || len + filterlen >= (int) sizeof(dst_hosts))
        fatal(str_ran32_29_hosts_1500446510);
      filterlen += len;
    }
    len = Snprintf(dst_hosts + filterlen, sizeof(dst_hosts) - filterlen, str_41414_3_14141_1535566263);
    if (len < 0 || len + filterlen >= (int) sizeof(dst_hosts))
      fatal(str_ran32_29_hosts_1500446510);
  }

  /* Open a network interface for packet capture */
  HOS->pd = my_pcap_open_live(Targets[0]->deviceName(), 8192,
    o.spoofsource ? 1 : 0, pcap_selectable_fd_valid() ? 200 : 2);
  if (HOS->pd == NULL)
    fatal(str_37s_2_37s_50831, PCAP_OPEN_ERRMSG);

  struct sockaddr_storage ss = Targets[0]->source();
  /* Build the final BPF filter */
  if (ss.ss_family == AF_INET) {
    if (doIndividual)
      len = Snprintf(pcap_filter, sizeof(pcap_filter), str_dst32_37_4037s_1021681524,
                   inet_ntoa(((struct sockaddr_in *)&ss)->sin_addr), dst_hosts);
    else
      len = Snprintf(pcap_filter, sizeof(pcap_filter), str_dst32_29_tcp41_736075727,
                   inet_ntoa(((struct sockaddr_in *)&ss)->sin_addr));
    if (len < 0 || len >= (int) sizeof(pcap_filter))
      fatal(str_ran32_31_ilter_2025193309);

    /* Compile and apply the filter to the pcap descriptor */
    if (o.debugging)
      log_write(LOG_PLAIN, str_Packe_39_7s92n_642119566, Targets[0]->deviceFullName(), pcap_filter);
    set_pcap_filter(Targets[0]->deviceFullName(), HOS->pd, pcap_filter);
  }

  return;
}


/* Sets everything up so the current round can be performed. This includes
 * reinitializing some variables of the supplied objects and deleting
 * some old information. */
static void startRound(OsScanInfo *OSI, HostOsScan *HOS, int roundNum) {
  std::list<HostOsScanInfo *>::iterator hostI;
  HostOsScanInfo *hsi = NULL;

  /* Reinitial some parameters of the scan system. */
  HOS->reInitScanSystem();

  for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI++) {
    hsi = *hostI;
    if (hsi->FPs[roundNum]) {
      delete hsi->FPs[roundNum];
      hsi->FPs[roundNum] = NULL;
    }
    hsi->hss->initScanStats();
  }
}

/* Run the sequence generation tests (6 TCP probes sent 100ms apart) */
static void doSeqTests(OsScanInfo *OSI, HostOsScan *HOS) {
  std::list<HostOsScanInfo *>::iterator hostI;
  HostOsScanInfo *hsi = NULL;
  HostOsScanStats *hss = NULL;
  unsigned int unableToSend = 0;  /* # of times in a row that hosts were unable to send probe */
  unsigned int expectReplies = 0;
  long to_usec = 0;
  int timeToSleep = 0;
  struct ip *ip = NULL;
  struct link_header linkhdr;
  struct sockaddr_storage ss;
  unsigned int bytes = 0;
  struct timeval rcvdtime;
  struct timeval stime;
  struct timeval tmptv;
  bool timedout = false;
  bool thisHostGood = false;
  bool foundgood = false;
  bool goodResponse = false;
  int numProbesLeft = 0;

  memset(&stime, 0, sizeof(stime));
  memset(&tmptv, 0, sizeof(tmptv));

  /* For each host, build a list of sequence probes to send */
  for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI++) {
    hsi = *hostI;
    hss = hsi->hss;
    HOS->buildSeqProbeList(hss);
  }

  /* Iterate until we have sent all the probes */
  do {
    if (timeToSleep > 0) {
      if (o.debugging > 1)
        log_write(LOG_PLAIN, str_Sleep_36_be92n_1200170776, timeToSleep);
      usleep(timeToSleep);
    }

    gettimeofday(&now, NULL);
    expectReplies = 0;
    unableToSend = 0;

    if (o.debugging > 2) {
      for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI++) {
        hss = (*hostI)->hss;
        log_write(LOG_PLAIN, str_Host3_45_7d92n_218651324,
                  hss->target->targetipstr(), hss->numProbesToSend(),
                  hss->numProbesActive());
      }
    }

    /* Send a seq probe to each host. */
    while (unableToSend < OSI->numIncompleteHosts() && HOS->stats->sendOK()) {
      hsi = OSI->nextIncompleteHost();
      hss = hsi->hss;
      gettimeofday(&now, NULL);
      if (hss->numProbesToSend()>0 && HOS->hostSeqSendOK(hss, NULL)) {
        HOS->sendNextProbe(hss);
        expectReplies++;
        unableToSend = 0;
      } else {
        unableToSend++;
      }
    }

    HOS->stats->num_probes_sent_at_last_wait = HOS->stats->num_probes_sent;

    gettimeofday(&now, NULL);

    /* Count the pcap wait time. */
    if (!HOS->stats->sendOK()) {
      TIMEVAL_MSEC_ADD(stime, now, 1000);

      for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI++) {
        if (HOS->nextTimeout((*hostI)->hss, &tmptv)) {
          if (TIMEVAL_SUBTRACT(tmptv, stime) < 0)
            stime = tmptv;
        }
      }
    } else {
      foundgood = false;
      for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI++) {
        thisHostGood = HOS->hostSeqSendOK((*hostI)->hss, &tmptv);
        if (thisHostGood) {
          stime = tmptv;
          foundgood = true;
          break;
        }

        if (!foundgood || TIMEVAL_SUBTRACT(tmptv, stime) < 0) {
          stime = tmptv;
          foundgood = true;
        }
      }
    }

    do {
      to_usec = TIMEVAL_SUBTRACT(stime, now);
      if (to_usec < 2000)
        to_usec = 2000;

      if (o.debugging > 2)
        log_write(LOG_PLAIN, str_pcap3_24_4692n_215852851, to_usec);

      ip = (struct ip*) readipv4_pcap(HOS->pd, &bytes, to_usec, &rcvdtime, &linkhdr, true);

      gettimeofday(&now, NULL);

      if (!ip && TIMEVAL_SUBTRACT(stime, now) < 0) {
        timedout = true;
        break;
      } else if (!ip) {
        continue;
      }

      if (TIMEVAL_SUBTRACT(now, stime) > 200000) {
        /* While packets are still being received, I'll be generous and give
           an extra 1/5 sec.  But we have to draw the line somewhere */
        timedout = true;
      }

      if (bytes < (4 * ip->ip_hl) + 4U)
        continue;

      memset(&ss, 0, sizeof(ss));
      ((struct sockaddr_in *) &ss)->sin_addr.s_addr = ip->ip_src.s_addr;
      ss.ss_family = AF_INET;
      hsi = OSI->findIncompleteHost(&ss);
      if (!hsi)
        continue; /* Not from one of our targets. */
      setTargetMACIfAvailable(hsi->target, &linkhdr, &ss, 0);

      goodResponse = HOS->processResp(hsi->hss, ip, bytes, &rcvdtime);

      if (goodResponse)
        expectReplies--;

    } while (!timedout && expectReplies > 0);

    /* Remove any timeout hosts during the scan. */
    OSI->removeCompletedHosts();

    numProbesLeft = 0;
    for (hostI = OSI->incompleteHosts.begin();
        hostI != OSI->incompleteHosts.end(); hostI++) {
      hss = (*hostI)->hss;
      HOS->updateActiveSeqProbes(hss);
      numProbesLeft += hss->numProbesToSend();
      numProbesLeft += hss->numProbesActive();
    }

    gettimeofday(&now, NULL);

    if (expectReplies == 0) {
      timeToSleep = TIMEVAL_SUBTRACT(stime, now);
    } else {
      timeToSleep = 0;
    }

  } while (numProbesLeft > 0);

}


/* TCP, UDP, ICMP Tests */
static void doTUITests(OsScanInfo *OSI, HostOsScan *HOS) {
  std::list<HostOsScanInfo *>::iterator hostI;
  HostOsScanInfo *hsi = NULL;
  HostOsScanStats *hss = NULL;
  unsigned int unableToSend; /* # of times in a row that hosts were unable to send probe */
  unsigned int expectReplies;
  long to_usec;
  int timeToSleep = 0;

  struct ip *ip = NULL;
  struct link_header linkhdr;
  struct sockaddr_storage ss;
  unsigned int bytes;
  struct timeval rcvdtime;

  struct timeval stime, tmptv;

  bool timedout = false;
  bool thisHostGood;
  bool foundgood;
  bool goodResponse;
  int numProbesLeft = 0;

  memset(&stime, 0, sizeof(stime));
  memset(&tmptv, 0, sizeof(tmptv));

  for (hostI = OSI->incompleteHosts.begin();
      hostI != OSI->incompleteHosts.end(); hostI++) {
    hsi = *hostI;
    hss = hsi->hss;
    HOS->buildTUIProbeList(hss);
  }

  do {

    if (timeToSleep > 0) {
      if (o.debugging > 1) {
        log_write(LOG_PLAIN, str_Time3_30_3292n_895931944, timeToSleep);
      }

      usleep(timeToSleep);
    }

    gettimeofday(&now, NULL);
    expectReplies = 0;
    unableToSend = 0;

    if (o.debugging > 2) {
      for (hostI = OSI->incompleteHosts.begin();
          hostI != OSI->incompleteHosts.end(); hostI++) {
        hss = (*hostI)->hss;
        log_write(LOG_PLAIN, str_Host3_45_7d92n_218651324,
                  hss->target->targetipstr(), hss->numProbesToSend(),
                  hss->numProbesActive());
      }
    }

    while (unableToSend < OSI->numIncompleteHosts() && HOS->stats->sendOK()) {
      hsi = OSI->nextIncompleteHost();
      hss = hsi->hss;
      gettimeofday(&now, NULL);
      if (hss->numProbesToSend()>0 && HOS->hostSendOK(hss, NULL)) {
        HOS->sendNextProbe(hss);
        expectReplies++;
        unableToSend = 0;
      } else {
        unableToSend++;
      }
    }

    HOS->stats->num_probes_sent_at_last_wait = HOS->stats->num_probes_sent;

    gettimeofday(&now, NULL);

    /* Count the pcap wait time. */
    if (!HOS->stats->sendOK()) {
      TIMEVAL_MSEC_ADD(stime, now, 1000);

      for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end();
          hostI++) {
        if (HOS->nextTimeout((*hostI)->hss, &tmptv)) {
          if (TIMEVAL_SUBTRACT(tmptv, stime) < 0)
            stime = tmptv;
        }
      }
    }
    else {
      foundgood = false;
      for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI++) {
        thisHostGood = HOS->hostSendOK((*hostI)->hss, &tmptv);
        if (thisHostGood) {
          stime = tmptv;
          foundgood = true;
          break;
        }

        if (!foundgood || TIMEVAL_SUBTRACT(tmptv, stime) < 0) {
          stime = tmptv;
          foundgood = true;
        }
      }
    }

    do {
      to_usec = TIMEVAL_SUBTRACT(stime, now);
      if (to_usec < 2000) to_usec = 2000;

      if (o.debugging > 2)
        log_write(LOG_PLAIN, str_pcap3_24_4692n_215852851, to_usec);

      ip = (struct ip*) readipv4_pcap(HOS->pd, &bytes, to_usec, &rcvdtime, &linkhdr, true);

      gettimeofday(&now, NULL);

      if (!ip && TIMEVAL_SUBTRACT(stime, now) < 0) {
        timedout = true;
        break;
      } else if (!ip) {
        continue;
      }

      if (TIMEVAL_SUBTRACT(now, stime) > 200000) {
        /* While packets are still being received, I'll be generous and give
           an extra 1/5 sec.  But we have to draw the line somewhere */
        timedout = true;
      }

      if (bytes < (4 * ip->ip_hl) + 4U)
        continue;

      memset(&ss, 0, sizeof(ss));
      ((struct sockaddr_in *) &ss)->sin_addr.s_addr = ip->ip_src.s_addr;
      ss.ss_family = AF_INET;
      hsi = OSI->findIncompleteHost(&ss);
      if (!hsi)
        continue; /* Not from one of our targets. */
      setTargetMACIfAvailable(hsi->target, &linkhdr, &ss, 0);

      goodResponse = HOS->processResp(hsi->hss, ip, bytes, &rcvdtime);

      if (goodResponse)
        expectReplies--;

    } while (!timedout && expectReplies > 0);

    /* Remove any timeout hosts during the scan. */
    OSI->removeCompletedHosts();

    numProbesLeft = 0;
    for (hostI = OSI->incompleteHosts.begin();
        hostI != OSI->incompleteHosts.end(); hostI++) {
      hss = (*hostI)->hss;
      HOS->updateActiveTUIProbes(hss);
      numProbesLeft += hss->numProbesToSend();
      numProbesLeft += hss->numProbesActive();
    }

    gettimeofday(&now, NULL);

    if (expectReplies == 0) {
      timeToSleep = TIMEVAL_SUBTRACT(stime, now);
    } else {
      timeToSleep = 0;
    }

  } while (numProbesLeft > 0);
}


static void endRound(OsScanInfo *OSI, HostOsScan *HOS, int roundNum) {
  std::list<HostOsScanInfo *>::iterator hostI;
  HostOsScanInfo *hsi = NULL;
  int distance = -1;
  enum dist_calc_method distance_calculation_method = DIST_METHOD_NONE;

  for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI++) {
    distance = -1;
    hsi = *hostI;
    HOS->makeFP(hsi->hss);

    hsi->FPs[roundNum] = hsi->hss->getFP();
    hsi->FPR->FPs[roundNum] = hsi->FPs[roundNum];
    hsi->FPR->numFPs = roundNum + 1;
    double tr = hsi->hss->timingRatio();
    hsi->target->FPR->maxTimingRatio = MAX(hsi->target->FPR->maxTimingRatio, tr);
    match_fingerprint(hsi->FPs[roundNum], &hsi->FP_matches[roundNum],
                      o.reference_FPs, OSSCAN_GUESS_THRESHOLD);

    if (hsi->FP_matches[roundNum].overall_results == OSSCAN_SUCCESS &&
        hsi->FP_matches[roundNum].num_perfect_matches > 0) {
      memcpy(&(hsi->target->seq), &hsi->hss->si, sizeof(struct seq_info));
      if (roundNum > 0) {
        if (o.verbose)
          log_write(LOG_STDOUT, str_WARNI_40_7d92n_947906427, roundNum + 1);
      }
      match_fingerprint(hsi->FPR->FPs[roundNum], hsi->FPR,
                        o.reference_FPs, OSSCAN_GUESS_THRESHOLD);
      hsi->isCompleted = true;
    }

    if (islocalhost(hsi->target->TargetSockAddr())) {
      /* scanning localhost */
      distance = 0;
      distance_calculation_method = DIST_METHOD_LOCALHOST;
    } else if (hsi->target->MACAddress()) {
      /* on the same network segment */
      distance = 1;
      distance_calculation_method = DIST_METHOD_DIRECT;
    } else if (hsi->hss->distance!=-1) {
      distance = hsi->hss->distance;
      distance_calculation_method = DIST_METHOD_ICMP;
    }

    hsi->target->distance = hsi->target->FPR->distance = distance;
    hsi->target->distance_calculation_method = distance_calculation_method;
    hsi->target->FPR->distance_guess = hsi->hss->distance_guess;

  }
  OSI->removeCompletedHosts();
}


static void findBestFPs(OsScanInfo *OSI) {
  std::list<HostOsScanInfo *>::iterator hostI;
  HostOsScanInfo *hsi = NULL;
  int i;

  double bestacc;
  int bestaccidx;

  for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI++) {
    hsi = *hostI;
    memcpy(&(hsi->target->seq), &hsi->hss->si, sizeof(struct seq_info));

    /* Now lets find the best match */
    bestacc = 0;
    bestaccidx = 0;
    for (i = 0; i < hsi->FPR->numFPs; i++) {
      if (hsi->FP_matches[i].overall_results == OSSCAN_SUCCESS &&
          hsi->FP_matches[i].num_matches > 0 &&
          hsi->FP_matches[i].accuracy[0] > bestacc) {
        bestacc = hsi->FP_matches[i].accuracy[0];
        bestaccidx = i;
        if (hsi->FP_matches[i].num_perfect_matches)
          break;
      }
    }

    // Now we redo the match, since target->FPR has various data (such as
    // target->FPR->numFPs) which is not in FP_matches[bestaccidx].  This is
    // kinda ugly.
    match_fingerprint(hsi->FPR->FPs[bestaccidx], (FingerPrintResultsIPv4 *) hsi->target->FPR,
                      o.reference_FPs, OSSCAN_GUESS_THRESHOLD);
  }
}


static void printFP(OsScanInfo *OSI) {
  std::list<HostOsScanInfo *>::const_iterator hostI;
  const HostOsScanInfo *hsi = NULL;
  const FingerPrintResultsIPv4 *FPR;

  for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI++) {
    hsi = *hostI;
    FPR = hsi->FPR;

    log_write(LOG_NORMAL|LOG_SKID_NOXLT|LOG_STDOUT,
          str_No32O_70_2n37s_1207463579,
          hsi->target->targetipstr(),
          mergeFPs(FPR->FPs, FPR->numFPs, true,
               hsi->target->TargetSockAddr(), hsi->target->distance,
               hsi->target->distance_calculation_method,
               hsi->target->MACAddress(),
               FPR->osscan_opentcpport, FPR->osscan_closedtcpport,
               FPR->osscan_closedudpport, false));
  }
}


/* Goes through every unmatched host in OSI.  If a host has completed
   the maximum number of OS detection tries allowed for it without
   matching, it is transferred to the passed in unMatchedHosts list.
   Returns the number of hosts moved to unMatchedHosts. */
static int expireUnmatchedHosts(OsScanInfo *OSI, std::list<HostOsScanInfo *> *unMatchedHosts) {
  std::list<HostOsScanInfo *>::iterator hostI, nextHost;
  int hostsRemoved = 0;
  HostOsScanInfo *HOS;

  gettimeofday(&now, NULL);
  for (hostI = OSI->incompleteHosts.begin(); hostI != OSI->incompleteHosts.end(); hostI = nextHost) {
    HOS = *hostI;
    nextHost = hostI;
    nextHost++;

    int max_tries = o.maxOSTries(); /* The amt. if print is suitable for submission */
    if (HOS->target->FPR->OmitSubmissionFP())
      max_tries = MIN(max_tries, STANDARD_OS2_TRIES);

    if (HOS->FPR->numFPs >= max_tries) {
      /* We've done all the OS2 tries we're going to do ... move this
     to unMatchedHosts */
      HOS->target->stopTimeOutClock(&now);
      OSI->incompleteHosts.erase(hostI);
      /* We need to adjust nextI if necessary */
      OSI->resetHostIterator();
      hostsRemoved++;
      unMatchedHosts->push_back(HOS);
    }
  }
  return hostsRemoved;
}


/******************************************************************************
 * Implementation of class OFProbe                                            *
 ******************************************************************************/

OFProbe::OFProbe() {
  type = OFP_UNSET;
  subid = 0;
  tryno = -1;
  retransmitted = false;
  memset(&sent, 0, sizeof(sent));
  memset(&prevSent, 0, sizeof(prevSent));
}


const char *OFProbe::typestr() const {
  switch (type) {
  case OFP_UNSET:
    return str_OFP95_9_UNSET_1975200268;
  case OFP_TSEQ:
    return str_OFP95_8_5TSEQ_351900448;
  case OFP_TOPS:
    return str_OFP95_8_5TOPS_351896947;
  case OFP_TECN:
    return str_OFP95_8_5TECN_351886929;
  case OFP_T1_7:
    return str_OFP95_8_T1957_1977013239;
  case OFP_TUDP:
    return str_OFP95_8_5TUDP_351902338;
  case OFP_TICMP:
    return str_OFP95_9_TICMP_1976287876;
  default:
    assert(false);
    return str_ERROR_5_ERROR_66247144;
  }
}


/******************************************************************************
 * Implementation of class HostOsScanStats                                    *
 ******************************************************************************/

HostOsScanStats::HostOsScanStats(Target * t) {
  int i;

  target = t;
  FP = NULL;

  memset(&si, 0, sizeof(si));
  memset(&ipid, 0, sizeof(ipid));

  openTCPPort = -1;
  closedTCPPort = -1;
  closedUDPPort = -1;

  num_probes_sent = 0;
  sendDelayMs = MAX(o.scan_delay, OS_PROBE_DELAY);
  lastProbeSent = now;

  /* Timing */
  timing.cwnd = perf.host_initial_cwnd;
  timing.ssthresh = perf.initial_ssthresh; /* Will be reduced if any packets are dropped anyway */
  timing.num_replies_expected = 0;
  timing.num_replies_received = 0;
  timing.num_updates = 0;
  gettimeofday(&timing.last_drop, NULL);

  for (i = 0; i < NUM_FPTESTS; i++)
    FPtests[i] = NULL;
  for (i = 0; i < 6; i++) {
    TOps_AVs[i] = NULL;
    TWin_AVs[i] = NULL;
  }

  icmpEchoReply = NULL;

  distance = -1;
  distance_guess = -1;
}


HostOsScanStats::~HostOsScanStats() {
  int i;

  for (i = 0; i < NUM_FPTESTS; i++) {
    if (FPtests[i] != NULL) {
      delete FPtests[i];
      FPtests[i] = NULL;
    }
  }

  while (!probesToSend.empty()) {
    delete probesToSend.front();
    probesToSend.pop_front();
  }

  while (!probesActive.empty()) {
    delete probesActive.front();
    probesActive.pop_front();
  }

  if (icmpEchoReply) free(icmpEchoReply);
}


void HostOsScanStats::initScanStats() {
  Port *tport = NULL;
  Port port;
  int i;

  /* Lets find an open port to use if we don't already have one */
  openTCPPort = -1;
  /*  target->FPR->osscan_opentcpport = -1;
  target->FPR->osscan_closedtcpport = -1;
  target->FPR->osscan_closedudpport = -1; */

  if (target->FPR->osscan_opentcpport > 0)
    openTCPPort = target->FPR->osscan_opentcpport;
  else if ((tport = target->ports.nextPort(NULL, &port, IPPROTO_TCP, PORT_OPEN))) {
    openTCPPort = tport->portno;
    /* If it is zero, let's try another one if there is one ) */
    if (tport->portno == 0)
      if ((tport = target->ports.nextPort(tport, &port, IPPROTO_TCP, PORT_OPEN)))
        openTCPPort = tport->portno;

    target->FPR->osscan_opentcpport = openTCPPort;
  }

  /* We should look at a different port if we know that this port is tcpwrapped */
  if (o.servicescan && openTCPPort > 0 && target->ports.isTCPwrapped(openTCPPort)) {
    if (o.debugging) {
      log_write(LOG_STDOUT, str_First_45_d4632_1860984219, openTCPPort);
    }
    /* Keep moving to other ports until we find one which is not tcpwrapped, or until we run out of ports */
    while ((tport = target->ports.nextPort(tport, &port, IPPROTO_TCP, PORT_OPEN))) {
      openTCPPort = tport->portno;
      if (!target->ports.isTCPwrapped(openTCPPort)) {
        break;
      }
    }

    target->FPR->osscan_opentcpport = openTCPPort;

    if (o.debugging) {
      if (target->ports.isTCPwrapped(openTCPPort)) {
        log_write(LOG_STDOUT, str_All32_110_4692n_643917166, openTCPPort);
      } else {
        log_write(LOG_STDOUT, str_Using_48_4692n_1556200408, openTCPPort);
      }
    }
  }

  /* Now we should find a closed TCP port */
  if (target->FPR->osscan_closedtcpport > 0)
    closedTCPPort = target->FPR->osscan_closedtcpport;
  else if ((tport = target->ports.nextPort(NULL, &port, IPPROTO_TCP, PORT_CLOSED))) {
    closedTCPPort = tport->portno;

    /* If it is zero, let's try another one if there is one ) */
    if (tport->portno == 0)
      if ((tport = target->ports.nextPort(tport, &port, IPPROTO_TCP, PORT_CLOSED)))
        closedTCPPort = tport->portno;

    target->FPR->osscan_closedtcpport = closedTCPPort;
  } else if ((tport = target->ports.nextPort(NULL, &port, IPPROTO_TCP, PORT_UNFILTERED))) {
    /* Well, we will settle for unfiltered */
    closedTCPPort = tport->portno;
    /* But again we'd prefer not to have zero */
    if (tport->portno == 0)
      if ((tport = target->ports.nextPort(tport, &port, IPPROTO_TCP, PORT_UNFILTERED)))
        closedTCPPort = tport->portno;
  } else {
    /* We'll just have to pick one at random :( */
    closedTCPPort = (get_random_uint() % 14781) + 30000;
  }

  /* Now we should find a closed UDP port */
  if (target->FPR->osscan_closedudpport > 0)
    closedUDPPort = target->FPR->osscan_closedudpport;
  else if ((tport = target->ports.nextPort(NULL, &port, IPPROTO_UDP, PORT_CLOSED))) {
    closedUDPPort = tport->portno;
    /* Not zero, if possible */
    if (tport->portno == 0)
      if ((tport = target->ports.nextPort(tport, &port, IPPROTO_UDP, PORT_CLOSED)))
        closedUDPPort = tport->portno;
    target->FPR->osscan_closedudpport = closedUDPPort;
  } else if ((tport = target->ports.nextPort(NULL, &port, IPPROTO_UDP, PORT_UNFILTERED))) {
    /* Well, we will settle for unfiltered */
    closedUDPPort = tport->portno;
    /* But not zero, please */
    if (tport->portno == 0)
      if ((tport = target->ports.nextPort(NULL, &port, IPPROTO_UDP, PORT_UNFILTERED)))
        closedUDPPort = tport->portno;
  } else {
    /* Pick one at random.  Shrug. */
    closedUDPPort = (get_random_uint() % 14781) + 30000;
  }

  FP = NULL;
  for (i = 0; i < NUM_FPTESTS; i++) {
    if (FPtests[i] != NULL) {
      delete FPtests[i];
      FPtests[i] = NULL;
    }
  }
  for (i = 0; i < 6; i++) {
    TOps_AVs[i] = NULL;
    TWin_AVs[i] = NULL;
  }

  TOpsReplyNum = 0;
  TWinReplyNum = 0;

  lastipid = 0;
  memset(&si, 0, sizeof(si));

  for (i = 0; i < NUM_SEQ_SAMPLES; i++) {
    ipid.tcp_ipids[i] = -1;
    ipid.tcp_closed_ipids[i] = -1;
    ipid.icmp_ipids[i] = -1;
  }

  memset(&seq_send_times, 0, sizeof(seq_send_times));

  if (icmpEchoReply) {
    free(icmpEchoReply);
    icmpEchoReply = NULL;
  }
  storedIcmpReply = -1;

  memset(&upi, 0, sizeof(upi));
}


/* Fill in an eth_nfo struct with the appropriate source and destination MAC
   addresses and a given Ethernet handle. The return value is suitable to pass
   to send_ip_packet: if ethsd is NULL, returns NULL; otherwise returns eth. */
struct eth_nfo *HostOsScanStats::fill_eth_nfo(struct eth_nfo *eth, eth_t *ethsd) const {
  if (ethsd == NULL)
    return NULL;

  memcpy(eth->srcmac, target->SrcMACAddress(), sizeof(eth->srcmac));
  memcpy(eth->dstmac, target->NextHopMACAddress(), sizeof(eth->srcmac));
  eth->ethsd = ethsd;
  eth->devname[0] = '\0';

  return eth;
}


/* Add a probe to the probe list. */
void HostOsScanStats::addNewProbe(OFProbeType type, int subid) {
  OFProbe *probe = new OFProbe();
  probe->type = type;
  probe->subid = subid;
  probesToSend.push_back(probe);
}


/* Remove a probe from the probesActive. */
void HostOsScanStats::removeActiveProbe(std::list<OFProbe *>::iterator probeI) {
  OFProbe *probe = *probeI;
  probesActive.erase(probeI);
  delete probe;
}


/* Get an active probe from active probe list identified by probe type
   and subid.  Returns probesActive.end() if there isn't one */
std::list<OFProbe *>::iterator HostOsScanStats::getActiveProbe(OFProbeType type, int subid) {
  std::list<OFProbe *>::iterator probeI;
  OFProbe *probe = NULL;

  for (probeI = probesActive.begin(); probeI != probesActive.end(); probeI++) {
    probe = *probeI;
    if (probe->type == type && probe->subid == subid)
      break;
  }

  if (probeI == probesActive.end()) {
    /* not found!? */
    if (o.debugging > 1)
      log_write(LOG_PLAIN, str_Probe_54_7d92n_1072080952, type, subid);
    return probesActive.end();
  }

  return probeI;
}


/* Move a probe from probesToSend to probesActive. */
void HostOsScanStats::moveProbeToActiveList(std::list<OFProbe *>::iterator probeI) {
  probesActive.push_back(*probeI);
  probesToSend.erase(probeI);
}


/* Move a probe from probesActive to probesToSend. */
void HostOsScanStats::moveProbeToUnSendList(std::list<OFProbe *>::iterator probeI) {
  probesToSend.push_back(*probeI);
  probesActive.erase(probeI);
}


 /* Compute the ratio of amount of time taken between sending 1st TSEQ
    probe and 1st ICMP probe compared to the amount of time it should
    have taken.  Ratios far from 1 can cause bogus results */
double HostOsScanStats::timingRatio() const {
  if (openTCPPort < 0)
    return 0;
  int msec_ideal = OS_SEQ_PROBE_DELAY * (NUM_SEQ_SAMPLES - 1);
  int msec_taken = TIMEVAL_MSEC_SUBTRACT(seq_send_times[NUM_SEQ_SAMPLES -1 ], seq_send_times[0]);
  if (o.debugging) {
    log_write(LOG_PLAIN, str_OS32d_65_3f92n_1170469861,
              seq_send_times[NUM_SEQ_SAMPLES - 1].tv_sec + seq_send_times[NUM_SEQ_SAMPLES - 1].tv_usec / 1000000.0, seq_send_times[0].tv_sec + (float) seq_send_times[0].tv_usec / 1000000.0, msec_ideal, (float) msec_taken / msec_ideal);
  }
  return (double) msec_taken / msec_ideal;
}


/******************************************************************************
 * Implementation of class HostOsScan                                         *
 ******************************************************************************/

/* If there are pending probe timeouts, fills in when with the time of
 * the earliest one and returns true.  Otherwise returns false and
 * puts now in when. */
bool HostOsScan::nextTimeout(HostOsScanStats *hss, struct timeval *when) const {
  assert(hss);
  struct timeval probe_to, earliest_to;
  std::list<OFProbe *>::const_iterator probeI;
  bool firstgood = true;

  assert(when);
  memset(&probe_to, 0, sizeof(probe_to));
  memset(&earliest_to, 0, sizeof(earliest_to));

  for (probeI = hss->probesActive.begin(); probeI != hss->probesActive.end(); probeI++) {
    TIMEVAL_ADD(probe_to, (*probeI)->sent, timeProbeTimeout(hss));
    if (firstgood || TIMEVAL_SUBTRACT(probe_to, earliest_to) < 0) {
      earliest_to = probe_to;
      firstgood = false;
    }
  }

  *when = (firstgood)? now : earliest_to;
  return !firstgood;
}


void HostOsScan::adjust_times(HostOsScanStats *hss, const OFProbe *probe, const struct timeval *rcvdtime) {
  assert(hss);
  assert(probe);

  /* Adjust timing */
  if (rcvdtime) {
    adjust_timeouts2(&(probe->sent), rcvdtime, &(hss->target->to));
    adjust_timeouts2(&(probe->sent), rcvdtime, &(stats->to));
  }

  stats->timing.num_replies_expected++;
  stats->timing.num_updates++;

  hss->timing.num_replies_expected++;
  hss->timing.num_updates++;

  /* Notice a drop if
     1. We get a response to a retransmitted probe (meaning the first reply was
        dropped), or
     2. We get no response after a timeout (rcvdtime == NULL). */
  if (probe->tryno > 0 || rcvdtime == NULL) {
    if (o.debugging > 1) {
      if (probe->tryno > 0) {
        log_write(LOG_PLAIN, str_OS32s_49_4192n_505820081,
          hss->target->targetipstr(), probe->tryno);
      } else {
        log_write(LOG_PLAIN, str_OS32s_57_4192n_2056547279,
          hss->target->targetipstr());
      }
    }
    if (TIMEVAL_AFTER(probe->sent, hss->timing.last_drop))
      hss->timing.drop(hss->numProbesActive(), &perf, &now);
    if (TIMEVAL_AFTER(probe->sent, stats->timing.last_drop))
      stats->timing.drop_group(stats->num_probes_active, &perf, &now);
  }

  /* Increase the window for a positive reply. This can overlap with case (1)
     above. */
  if (rcvdtime != NULL) {
    stats->timing.ack(&perf);
    hss->timing.ack(&perf);
  }
}


HostOsScan::HostOsScan(Target *t) {
  pd = NULL;
  rawsd = -1;
  ethsd = NULL;

  if ((o.sendpref & PACKET_SEND_ETH) && (t->ifType() == devt_ethernet
#ifdef WIN32
    || (g_has_npcap_loopback && t->ifType() == devt_loopback)
#endif
    )) {
    if ((ethsd = eth_open_cached(t->deviceName())) == NULL)
      fatal(str_37s58_39_37s41_2031883275, __func__, t->deviceName());
    rawsd = -1;
  } else {
#ifdef WIN32
    win32_fatal_raw_sockets(t->deviceName());
#endif
    rawsd = nmap_raw_socket();
    if (rawsd < 0)
      pfatal(str_socke_21_3237s_1509366238, __func__);
    unblock_socket(rawsd);
    ethsd = NULL;
  }

  tcpPortBase = o.magic_port_set? o.magic_port : o.magic_port + get_random_u8();
  udpPortBase = o.magic_port_set? o.magic_port : o.magic_port + get_random_u8();
  reInitScanSystem();

  stats = new ScanStats();
}


HostOsScan::~HostOsScan() {
  if (rawsd >= 0) {
    close(rawsd);
    rawsd = -1;
  }
  if (pd) {
    pcap_close(pd);
    pd = NULL;
  }
  /* No need to close ethsd due to caching. */
  delete stats;
}


void HostOsScan::reInitScanSystem() {
  tcpSeqBase = get_random_u32();
  tcpAck = get_random_u32();
  tcpMss = 265;
  icmpEchoId = get_random_u16();
  icmpEchoSeq = 295;
  udpttl = (time(NULL) % 14) + 51;
}


/* Initiate seq probe list */
void HostOsScan::buildSeqProbeList(HostOsScanStats *hss) {
  assert(hss);
  int i;
  if (hss->openTCPPort == -1)
    return;
  if (hss->FP_TSeq)
    return;

  for (i = 0; i < NUM_SEQ_SAMPLES; i++)
    hss->addNewProbe(OFP_TSEQ, i);
}


/* Update the seq probes in the active probe list and remove the ones that have
 * timed out. */
void HostOsScan::updateActiveSeqProbes(HostOsScanStats *hss) {
  assert(hss);
  std::list<OFProbe *>::iterator probeI, nxt;
  OFProbe *probe = NULL;

  for (probeI = hss->probesActive.begin(); probeI != hss->probesActive.end(); probeI = nxt) {
    nxt = probeI;
    nxt++;
    probe = *probeI;

    /* Is the probe timedout? */
    if (TIMEVAL_SUBTRACT(now, probe->sent) > (long) timeProbeTimeout(hss)) {
      hss->removeActiveProbe(probeI);
      assert(stats->num_probes_active > 0);
      stats->num_probes_active--;
    }
  }
}


/* Initialize the normal TCP/UDP/ICMP probe list */
void HostOsScan::buildTUIProbeList(HostOsScanStats *hss) {
  assert(hss);
  int i;

  /* The order of these probes are important for ipid generation
   * algorithm test and should not be changed.
   *
   * At doSeqTests we sent 6 TSeq probes to generate 6 tcp replies,
   * and here we follow with 3 probes to generate 3 icmp replies. In
   * this way we can expect to get "good" IPid sequence.
   *
   * **** Should be done in a more elegant way. *****
   */

  /* ticmp */
  if (!hss->FP_TIcmp) {
    for (i = 0; i < 2; i++) {
      hss->addNewProbe(OFP_TICMP, i);
    }
  }

  /* tudp */
  if (!hss->FP_TUdp) {
    hss->addNewProbe(OFP_TUDP, 0);
  }

  if (hss->openTCPPort != -1) {
    /* tops/twin probes. We send the probe again if we didn't get a
       response by the corresponding seq probe. */
    if (!hss->FP_TOps || !hss->FP_TWin) {
      for (i = 0; i < 6; i++) {
        if (!hss->TOps_AVs[i] || !hss->TWin_AVs[i])
          hss->addNewProbe(OFP_TOPS, i);
      }
    }

    /* tecn */
    if (!hss->FP_TEcn) {
      hss->addNewProbe(OFP_TECN, 0);
    }

    /* t1_7: t1_t4 */
    for (i = 0; i < 4; i++) {
      if (!hss->FPtests[FP_T1_7_OFF + i]) {
        hss->addNewProbe(OFP_T1_7, i);
      }
    }
  }

  /* t1_7: t5_t7 */
  for (i = 4; i < 7; i++) {
    if (!hss->FPtests[FP_T1_7_OFF + i]) {
      hss->addNewProbe(OFP_T1_7, i);
    }
  }
}


/* Update the probes in the active probe list:
 * 1) Remove the expired probes (timedout and reached the retry limit);
 * 2) Move timedout probes to probeNeedToSend; */
void HostOsScan::updateActiveTUIProbes(HostOsScanStats *hss) {
  assert(hss);
  std::list<OFProbe *>::iterator probeI, nxt;
  OFProbe *probe = NULL;

  for (probeI = hss->probesActive.begin(); probeI != hss->probesActive.end(); probeI = nxt) {
    nxt = probeI;
    nxt++;
    probe = *probeI;

    if (TIMEVAL_SUBTRACT(now, probe->sent) > (long) timeProbeTimeout(hss)) {
      if (probe->tryno >= 3) {
        /* The probe is expired. */
        hss->removeActiveProbe(probeI);
        assert(stats->num_probes_active > 0);
        stats->num_probes_active--;
      }
      else {
        /* It is timedout, move it to the sendlist */
        hss->moveProbeToUnSendList(probeI);
        assert(stats->num_probes_active > 0);
        stats->num_probes_active--;
      }
    }
  }
}


/* Check whether the host is sendok. If not, fill _when_ with the time
 * when it will be sendOK and return false; else, fill it with now and
 * return true. */
bool HostOsScan::hostSendOK(HostOsScanStats *hss, struct timeval *when) const {
  assert(hss);
  std::list<OFProbe *>::const_iterator probeI;
  int packTime;
  struct timeval probe_to, earliest_to, sendTime;
  long tdiff;

  if (hss->target->timedOut(&now)) {
    if (when)
      *when = now;
    return false;
  }

  if (hss->sendDelayMs > 0) {
    packTime = TIMEVAL_MSEC_SUBTRACT(now, hss->lastProbeSent);
    if (packTime < (int) hss->sendDelayMs) {
      if (when) {
        TIMEVAL_MSEC_ADD(*when, hss->lastProbeSent, hss->sendDelayMs);
      }
      return false;
    }
  }

  if (hss->timing.cwnd >= hss->numProbesActive() + .5) {
    if (when)
      *when = now;
    return true;
  }

  if (!when)
    return false;

  TIMEVAL_MSEC_ADD(earliest_to, now, 10000);

  /* Any timeouts coming up? */
  for (probeI = hss->probesActive.begin(); probeI != hss->probesActive.end(); probeI++) {
    TIMEVAL_MSEC_ADD(probe_to, (*probeI)->sent, timeProbeTimeout(hss) / 1000);
    if (TIMEVAL_SUBTRACT(probe_to, earliest_to) < 0) {
      earliest_to = probe_to;
    }
  }

  // Will any scan delay affect this?
  if (hss->sendDelayMs > 0) {
    TIMEVAL_MSEC_ADD(sendTime, hss->lastProbeSent, hss->sendDelayMs);
    if (TIMEVAL_MSEC_SUBTRACT(sendTime, now) < 0)
      sendTime = now;
    tdiff = TIMEVAL_MSEC_SUBTRACT(earliest_to, sendTime);

    /* Timeouts previous to the sendTime requirement are pointless,
       and those later than sendTime are not needed if we can send a
       new packet at sendTime */
    if (tdiff < 0) {
      earliest_to = sendTime;
    } else {
      if (tdiff > 0 && hss->timing.cwnd > hss->numProbesActive() + .5) {
        earliest_to = sendTime;
      }
    }
  }

  *when = earliest_to;
  return false;
}


/* Check whether it is OK to send the next seq probe to the host. If
 * not, fill param "when" with the time when it will be sendOK and return
 * false; else, fill it with now and return true. */
bool HostOsScan::hostSeqSendOK(HostOsScanStats *hss, struct timeval *when) const {
  assert(hss);
  std::list<OFProbe *>::const_iterator probeI;
  int packTime = 0, maxWait = 0;
  struct timeval probe_to, earliest_to, sendTime;
  long tdiff;

  if (hss->target->timedOut(&now)) {
    if (when)
      *when = now;
    return false;
  }

  packTime = TIMEVAL_SUBTRACT(now, hss->lastProbeSent);

  /*
   * If the user insist a larger sendDelayMs, use it. But
   * the seq result may be inaccurate.
   */
  maxWait = MAX(OS_SEQ_PROBE_DELAY * 1000, hss->sendDelayMs * 1000);
  if (packTime < maxWait) {
    if (when) {
      TIMEVAL_ADD(*when, hss->lastProbeSent, maxWait);
    }
    return false;
  }

  if (hss->timing.cwnd >= hss->numProbesActive() + .5) {
    if (when)
      *when = now;
    return true;
  }

  if (!when)
    return false;

  TIMEVAL_MSEC_ADD(earliest_to, now, 10000);

  /* Any timeouts coming up? */
  for (probeI = hss->probesActive.begin(); probeI != hss->probesActive.end(); probeI++) {
    TIMEVAL_MSEC_ADD(probe_to, (*probeI)->sent, timeProbeTimeout(hss) / 1000);
    if (TIMEVAL_SUBTRACT(probe_to, earliest_to) < 0) {
      earliest_to = probe_to;
    }
  }

  TIMEVAL_ADD(sendTime, hss->lastProbeSent, maxWait);
  if (TIMEVAL_SUBTRACT(sendTime, now) < 0)
    sendTime = now;
  tdiff = TIMEVAL_SUBTRACT(earliest_to, sendTime);

  /* Timeouts previous to the sendTime requirement are pointless,
     and those later than sendTime are not needed if we can send a
     new packet at sendTime */
  if (tdiff < 0) {
    earliest_to = sendTime;
  } else {
    if (tdiff > 0 && hss->timing.cwnd > hss->numProbesActive() + .5) {
      earliest_to = sendTime;
    }
  }

  *when = earliest_to;
  return false;
}


unsigned long HostOsScan::timeProbeTimeout(HostOsScanStats *hss) const {
  assert(hss);
  if (hss->target->to.srtt > 0) {
    /* We have at least one timing value to use.  Good enough, I suppose */
    return hss->target->to.timeout;
  } else if (stats->to.srtt > 0) {
    /* OK, we'll use this one instead */
    return stats->to.timeout;
  } else {
    return hss->target->to.timeout; /* It comes with a default */
  }
}


void HostOsScan::sendNextProbe(HostOsScanStats *hss) {
  assert(hss);
  std::list<OFProbe *>::iterator probeI;
  OFProbe *probe = NULL;

  if (hss->probesToSend.empty())
    return;

  if (!hss->target->timeOutClockRunning() && !hss->target->timedOut(NULL)) {
    hss->target->startTimeOutClock(&now);
  }

  probeI = hss->probesToSend.begin();
  probe = *probeI;

  switch (probe->type) {
  case OFP_TSEQ:
    sendTSeqProbe(hss, probe->subid);
    break;
  case OFP_TOPS:
    sendTOpsProbe(hss, probe->subid);
    break;
  case OFP_TECN:
    sendTEcnProbe(hss);
    break;
  case OFP_T1_7:
    sendT1_7Probe(hss, probe->subid);
    break;
  case OFP_TICMP:
    sendTIcmpProbe(hss, probe->subid);
    break;
  case OFP_TUDP:
    sendTUdpProbe(hss, probe->subid);
    break;
  default:
    assert(false);
  }

  probe->tryno++;
  if (probe->tryno > 0) {
    /* This is a retransmission */
    probe->retransmitted = true;
    probe->prevSent = probe->sent;
  }
  probe->sent = now;

  hss->lastProbeSent = now;
  hss->num_probes_sent++;
  stats->num_probes_sent++;

  hss->moveProbeToActiveList(probeI);
  stats->num_probes_active++;

  if (o.debugging > 1) {
    log_write(LOG_PLAIN, str_Send3_40_7s92n_77114893,
              probe->typestr(), probe->subid, hss->target->targetipstr());
  }

}


void HostOsScan::sendTSeqProbe(HostOsScanStats *hss, int probeNo) {
  assert(hss);
  assert(probeNo >= 0 && probeNo < NUM_SEQ_SAMPLES);

  if (hss->openTCPPort == -1)
    return;

  send_tcp_probe(hss, o.ttl, false, NULL, 0,
                 tcpPortBase + probeNo, hss->openTCPPort,
                 tcpSeqBase + probeNo, tcpAck,
                 0, TH_SYN, prbWindowSz[probeNo], 0,
                 prbOpts[probeNo].val, prbOpts[probeNo].len, NULL, 0);

  hss->seq_send_times[probeNo] = now;
}


void HostOsScan::sendTOpsProbe(HostOsScanStats *hss, int probeNo) {
  assert(hss);
  assert(probeNo >= 0 && probeNo < NUM_SEQ_SAMPLES);

  if (hss->openTCPPort == -1)
    return;

  send_tcp_probe(hss, o.ttl, false, NULL, 0,
                 tcpPortBase + NUM_SEQ_SAMPLES + probeNo, hss->openTCPPort,
                 tcpSeqBase, tcpAck,
                 0, TH_SYN, prbWindowSz[probeNo], 0,
                 prbOpts[probeNo].val, prbOpts[probeNo].len, NULL, 0);
}


void HostOsScan::sendTEcnProbe(HostOsScanStats *hss) {
  assert(hss);

  if (hss->openTCPPort == -1)
    return;

  send_tcp_probe(hss, o.ttl, false, NULL, 0,
                 tcpPortBase + NUM_SEQ_SAMPLES + 6, hss->openTCPPort,
                 tcpSeqBase, 0,
                 8, TH_CWR|TH_ECE|TH_SYN, prbWindowSz[6], 63477,
                 prbOpts[6].val, prbOpts[6].len, NULL, 0);
}


void HostOsScan::sendT1_7Probe(HostOsScanStats *hss, int probeNo) {
  assert(hss);
  assert(probeNo >=0 && probeNo < 7);

  int port_base = tcpPortBase + NUM_SEQ_SAMPLES + 7;

  switch (probeNo) {
  case 0: /* T1 */
    /* T1 is normally filled in by sendTSeqProbe so this case doesn't happen. In
       case all six Seq probes failed, this one will be re-sent. It is the same
       as the first probe sent by sendTSeqProbe. */
    if (hss->openTCPPort == -1)
      return;
    send_tcp_probe(hss, o.ttl, false, NULL, 0,
                   port_base, hss->openTCPPort,
                   tcpSeqBase, tcpAck,
                   0, TH_SYN, prbWindowSz[0], 0,
                   prbOpts[0].val, prbOpts[0].len, NULL, 0);
    break;
  case 1: /* T2 */
    if (hss->openTCPPort == -1)
      return;
    send_tcp_probe(hss, o.ttl, true, NULL, 0,
                   port_base + 1, hss->openTCPPort,
                   tcpSeqBase, tcpAck,
                   0, 0, prbWindowSz[7], 0,
                   prbOpts[7].val, prbOpts[7].len, NULL, 0);
    break;
  case 2: /* T3 */
    if (hss->openTCPPort == -1)
      return;
    send_tcp_probe(hss, o.ttl, false, NULL, 0,
                   port_base + 2, hss->openTCPPort,
                   tcpSeqBase, tcpAck,
                   0, TH_SYN|TH_FIN|TH_URG|TH_PUSH, prbWindowSz[8], 0,
                   prbOpts[8].val, prbOpts[8].len, NULL, 0);
    break;
  case 3: /* T4 */
    if (hss->openTCPPort == -1)
      return;
    send_tcp_probe(hss, o.ttl, true, NULL, 0,
                   port_base + 3, hss->openTCPPort,
                   tcpSeqBase, tcpAck,
                   0, TH_ACK, prbWindowSz[9], 0,
                   prbOpts[9].val, prbOpts[9].len, NULL, 0);
    break;
  case 4: /* T5 */
    if (hss->closedTCPPort == -1)
      return;
    send_tcp_probe(hss, o.ttl, false, NULL, 0,
                   port_base + 4, hss->closedTCPPort,
                   tcpSeqBase, tcpAck,
                   0, TH_SYN, prbWindowSz[10], 0,
                   prbOpts[10].val, prbOpts[10].len, NULL, 0);
    break;
  case 5: /* T6 */
    if (hss->closedTCPPort == -1)
      return;
    send_tcp_probe(hss, o.ttl, true, NULL, 0,
                   port_base + 5, hss->closedTCPPort,
                   tcpSeqBase, tcpAck,
                   0, TH_ACK, prbWindowSz[11], 0,
                   prbOpts[11].val, prbOpts[11].len, NULL, 0);
    break;
  case 6: /* T7 */
    if (hss->closedTCPPort == -1)
      return;
    send_tcp_probe(hss, o.ttl, false, NULL, 0,
                   port_base + 6, hss->closedTCPPort,
                   tcpSeqBase, tcpAck,
                   0, TH_FIN|TH_PUSH|TH_URG, prbWindowSz[12], 0,
                   prbOpts[12].val, prbOpts[12].len, NULL, 0);
  }
}


void HostOsScan::sendTIcmpProbe(HostOsScanStats *hss, int probeNo) {
  assert(hss);
  assert(probeNo >= 0 && probeNo < 2);
  if (probeNo == 0) {
    send_icmp_echo_probe(hss, IP_TOS_DEFAULT,
                         true, 9, icmpEchoId, icmpEchoSeq, 120);
  }
  else {
    send_icmp_echo_probe(hss, IP_TOS_RELIABILITY,
                         false, 0, icmpEchoId + 1, icmpEchoSeq + 1, 150);
  }
}


void HostOsScan::sendTUdpProbe(HostOsScanStats *hss, int probeNo) {
  assert(hss);
  if (hss->closedUDPPort == -1)
    return;
  send_closedudp_probe(hss, udpttl, udpPortBase + probeNo, hss->closedUDPPort);
}


bool HostOsScan::processResp(HostOsScanStats *hss, const struct ip *ip, unsigned int len, struct timeval *rcvdtime) {
  const struct ip *ip2;
  const struct tcp_hdr *tcp;
  const struct icmp *icmp;
  int testno;
  bool isPktUseful = false;
  std::list<OFProbe *>::iterator probeI;
  OFProbe *probe;

  if (len < 20 || len < (4 * ip->ip_hl) + 4U)
    return false;

  len -= 4 * ip->ip_hl;

  if (ip->ip_p == IPPROTO_TCP) {
    if (len < 20)
      return false;
    tcp = ((struct tcp_hdr *) (((char *) ip) + 4 * ip->ip_hl));
    if (len < (unsigned int)(4 * tcp->th_off))
      return false;
    testno = ntohs(tcp->th_dport) - tcpPortBase;

    if (testno >= 0 && testno < NUM_SEQ_SAMPLES) {
      /* TSeq */
      isPktUseful = processTSeqResp(hss, ip, testno);

      if (isPktUseful) {
        hss->ipid.tcp_ipids[testno] = ntohs(ip->ip_id);
        probeI = hss->getActiveProbe(OFP_TSEQ, testno);
        /* printf("tcp ipid = %d\n", ntohs(ip->ip_id)); */
      }

      /* Use the seq response to do other tests. We don't care if it
       * is useful for these tests.
       */
      if (testno == 0) {
        /* the first reply is used to do T1 */
        processT1_7Resp(hss, ip, 0);
      }
      /* the 1st NUM_SEQ_SAMPLES replies are used to do TOps and TWin */
      processTOpsResp(hss, tcp, testno);
      processTWinResp(hss, tcp, testno);

    } else if (testno >= NUM_SEQ_SAMPLES && testno < NUM_SEQ_SAMPLES + 6) {

      /* TOps/Twin */
      isPktUseful = processTOpsResp(hss, tcp, testno - NUM_SEQ_SAMPLES);
      isPktUseful |= processTWinResp(hss, tcp, testno - NUM_SEQ_SAMPLES);
      if (isPktUseful) {
        probeI = hss->getActiveProbe(OFP_TOPS, testno - NUM_SEQ_SAMPLES);
      }

    } else if (testno == NUM_SEQ_SAMPLES + 6) {

      /* TEcn */
      isPktUseful = processTEcnResp(hss, ip);
      if (isPktUseful) {
        probeI = hss->getActiveProbe(OFP_TECN, 0);
      }

    } else if (testno >= NUM_SEQ_SAMPLES + 7 && testno < NUM_SEQ_SAMPLES + 14) {

      isPktUseful = processT1_7Resp(hss, ip, testno - NUM_SEQ_SAMPLES - 7);

      if (isPktUseful) {
        probeI = hss->getActiveProbe(OFP_T1_7, testno - NUM_SEQ_SAMPLES - 7);

        /* Closed-port TCP IP ID sequence numbers (SEQ.CI). Uses T5, T6, and T7.
           T5 starts at NUM_SEQ_SAMPLES + 11. */
        if (testno >= NUM_SEQ_SAMPLES + 11)
          hss->ipid.tcp_closed_ipids[testno - (NUM_SEQ_SAMPLES + 11)] = ntohs(ip->ip_id);
      }
    }
  }
  else if (ip->ip_p == IPPROTO_ICMP) {
    if (len < 8)
      return false;
    icmp = ((struct icmp *)(((char *) ip) + 4 * ip->ip_hl));

    /* Is it an icmp echo reply? */
    if (icmp->icmp_type == ICMP_ECHOREPLY) {
      testno = ntohs(icmp->icmp_id) - icmpEchoId;
      if (testno == 0 || testno == 1) {
        isPktUseful = processTIcmpResp(hss, ip, testno);
        if (isPktUseful) {
          probeI = hss->getActiveProbe(OFP_TICMP, testno);
        }

        if (isPktUseful && probeI != hss->probesActive.end() && !(*probeI)->retransmitted) { /* Retransmitted ipid is useless. */
          hss->ipid.icmp_ipids[testno] = ntohs(ip->ip_id);
          /* printf("icmp ipid = %d\n", ntohs(ip->ip_id)); */
        }
      }
    }

    /* Is it a destination port unreachable? */
    if (icmp->icmp_type == 3 && icmp->icmp_code == 3) {
      len -= 8; /* icmp destination unreachable header len. */
      if (len < 28)
        return false; /* must larger than an ip and an udp header length */
      ip2 = (struct ip*)((char *)icmp + 8);
      len -= 4 * ip2->ip_hl;
      if (len < 8)
        return false;

      isPktUseful = processTUdpResp(hss, ip);
      if (isPktUseful) {
        probeI = hss->getActiveProbe(OFP_TUDP, 0);
      }
    }

  }

  if (isPktUseful && probeI != hss->probesActive.end()) {
    probe = *probeI;

    if (rcvdtime)
      adjust_times(hss, probe, rcvdtime);

    if (o.debugging > 1) {
      log_write(LOG_PLAIN, str_Got32_61_7s92n_1883422633,
            probe->typestr(), probe->subid, hss->target->targetipstr());
    }

    /* delete the probe. */
    hss->removeActiveProbe(probeI);
    assert(stats->num_probes_active > 0);
    stats->num_probes_active--;

    return true;
  }

  return false;
}


void HostOsScan::makeFP(HostOsScanStats *hss) {
  assert(hss);

  int i;

  if (!hss->FP_TSeq)
    makeTSeqFP(hss);

  if (!hss->FP_TOps)
    makeTOpsFP(hss);

  if (!hss->FP_TWin)
    makeTWinFP(hss);

  for (i = 3; i < NUM_FPTESTS; i++) {
    if (!hss->FPtests[i] &&
        ((i >= 3 && i <= 7 && hss->openTCPPort != -1) ||
         (i >= 8 && i <= 10 && hss->target->FPR->osscan_closedtcpport != -1) ||
         i >= 11)) {
      /* We create a Resp (response) attribute with value of N (no) because
         it is important here to note whether responses were or were not
         received */
      hss->FPtests[i] = new FingerTest(INT2ID(i), *o.reference_FPs->MatchPoints);
      hss->FPtests[i]->setAVal(str_R_1_R_82, str_N_1_N_78);
    }
    else if (hss->FPtests[i]) {
      FingerTest &test = *hss->FPtests[i];
      /* The value for this attribute is the
       * received TTL. We replace it with the initial TTL. */
      const char *recvTTL = test.getAVal(str_T_1_T_84);
      if (recvTTL) {
        int ttl = strtol(recvTTL, NULL, 16);

        if (hss->distance_guess == -1)
          hss->distance_guess = get_initial_ttl_guess(ttl) - ttl;

        if (hss->distance != -1) {
          /* We've gotten response for the UDP probe and thus have
             the "true" hop count. Add the number of hops between
             us and the target (hss->distance - 1) to the received
             TTL to get the initial TTL. */
          test.setAVal(str_T_1_T_84, hss->target->FPR->cp_hex(ttl + hss->distance - 1));
        } else {
          /* Guess the initial TTL value */
          test.setAVal(str_TG_2_TG_2675, hss->target->FPR->cp_hex(get_initial_ttl_guess(ttl)));
          /* Delete this unknown-distance-dependent value */
          test.setAVal(str_T_1_T_84, NULL);
        }
      }
    }
  }

  /* Link them up. */
  hss->FP = new FingerPrint;
  for (i = 0; i < NUM_FPTESTS; i++) {
    if (hss->FPtests[i] == NULL)
      continue;
    hss->FP->tests[i] = *hss->FPtests[i];
  }
}


/* Send a TCP probe. This takes care of decoys and filling in Ethernet
 * addresses if necessary. Used for the SEQ, OPS, WIN, ECN, and T1-T7 probes. */
int HostOsScan::send_tcp_probe(HostOsScanStats *hss,
                               int ttl, bool df, u8* ipopt, int ipoptlen,
                               u16 sport, u16 dport, u32 seq, u32 ack,
                               u8 reserved, u8 flags, u16 window, u16 urp,
                               u8 *options, int optlen,
                               char *data, u16 datalen) {
  struct eth_nfo eth, *ethptr;

  ethptr = hss->fill_eth_nfo(&eth, ethsd);

  return send_tcp_raw_decoys(rawsd, ethptr, hss->target->v4hostip(),
                             ttl, df, ipopt, ipoptlen, sport, dport, seq, ack,
                             reserved, flags, window, urp,
                             options, optlen, data, datalen);
}


/* Send an echo probe. This takes care of decoys and filling in Ethernet
 * addresses if necessary. Used for the IE probes. */
int HostOsScan::send_icmp_echo_probe(HostOsScanStats *hss,
                                     u8 tos, bool df, u8 pcode,
                                     unsigned short id, u16 seq, u16 datalen) {
  u8 *packet = NULL;
  u32 packetlen = 0;
  int decoy;
  int res = -1;
  struct eth_nfo eth, *ethptr;

  ethptr = hss->fill_eth_nfo(&eth, ethsd);

  for (decoy = 0; decoy < o.numdecoys; decoy++) {
    packet = build_icmp_raw(&((struct sockaddr_in *)&o.decoys[decoy])->sin_addr, hss->target->v4hostip(),
                            o.ttl, get_random_u16(), tos, df, NULL, 0, seq, id,
                            ICMP_ECHO, pcode, NULL, datalen, &packetlen);
    if (!packet)
      return -1;
    res = send_ip_packet(rawsd, ethptr, hss->target->TargetSockAddr(), packet, packetlen);
    free(packet);
    if (res == -1)
      return -1;
  }

  return 0;
}


/* Send a UDP probe. This takes care of decoys and filling in Ethernet
 * addresses if necessary. Used for the U1 probe. */
int HostOsScan::send_closedudp_probe(HostOsScanStats *hss,
                                     int ttl, u16 sport, u16 dport) {
  static int myttl = 0;
  static u8 patternbyte = 0x43; /* character 'C' */
  static u16 id = 0x1042;
  u8 packet[328]; /* 20 IP hdr + 8 UDP hdr + 300 data */
  struct ip *ip = (struct ip *) packet;
  struct udp_hdr *udp = (struct udp_hdr *) (packet + sizeof(struct ip));
  struct in_addr *source;
  int datalen = 300;
  unsigned char *data = packet + 28;
  unsigned short realcheck; /* the REAL checksum */
  int res;
  int decoy;
  struct eth_nfo eth, *ethptr;

  ethptr = hss->fill_eth_nfo(&eth, ethsd);

  /* if (!patternbyte) patternbyte = (get_random_uint() % 60) + 65; */
  memset(data, patternbyte, datalen);

  /*  while (!id) id = get_random_uint(); */

  if (ttl == -1) {
    myttl = (time(NULL) % 14) + 51;
  } else {
    myttl = ttl;
  }

  /* check that required fields are there and not too silly */
  if (!sport || !dport) {
    error(str_37s58_40_uck33_1831161883, __func__);
    return 1;
  }

  for (decoy = 0; decoy < o.numdecoys; decoy++) {
    if (o.decoys[decoy].ss_family == AF_INET6)
      return 1;
    source = &((struct sockaddr_in *)&o.decoys[decoy])->sin_addr;

    memset((char *) packet, 0, sizeof(struct ip) + sizeof(struct udp_hdr));

    udp->uh_sport = htons(sport);
    udp->uh_dport = htons(dport);
    udp->uh_ulen = htons(8 + datalen);

    /* OK, now we should be able to compute a valid checksum */
    realcheck = ipv4_pseudoheader_cksum(source, hss->target->v4hostip(), IPPROTO_UDP,
                                        sizeof(struct udp_hdr) + datalen, (char *) udp);
#if STUPID_SOLARIS_CHECKSUM_BUG
    udp->uh_sum = sizeof(struct udp_hdr) + datalen;
#else
    udp->uh_sum = realcheck;
#endif

    /* Now for the ip header */
    ip->ip_v = 4;
    ip->ip_hl = 5;
    ip->ip_len = htons(sizeof(struct ip) + sizeof(struct udp_hdr) + datalen);
    ip->ip_id = htons(id);
    ip->ip_ttl = myttl;
    ip->ip_p = IPPROTO_UDP;
    ip->ip_src.s_addr = source->s_addr;
    ip->ip_dst.s_addr= hss->target->v4hostip()->s_addr;

    hss->upi.ipck = in_cksum((unsigned short *)ip, sizeof(struct ip));
#if HAVE_IP_IP_SUM
    ip->ip_sum = hss->upi.ipck;
#endif

    /* OK, now if this is the real she-bang (ie not a decoy) then
       we stick all the inph0 in our upi */
    if (decoy == o.decoyturn) {
      hss->upi.iptl = 28 + datalen;
      hss->upi.ipid = id;
      hss->upi.sport = sport;
      hss->upi.dport = dport;
      hss->upi.udpck = realcheck;
      hss->upi.udplen = 8 + datalen;
      hss->upi.patternbyte = patternbyte;
      hss->upi.target.s_addr = ip->ip_dst.s_addr;
    }

    if ((res = send_ip_packet(rawsd, ethptr, hss->target->TargetSockAddr(), packet, ntohs(ip->ip_len))) == -1)
      {
        gh_perror(str_send9_20_3237s_256041725, __func__);
        return 1;
      }
  }

  return 0;
}


/******************************************************************************
 * Implementation of class ScanStats                                          *
 ******************************************************************************/

ScanStats::ScanStats() {
  /* init timing val */
  timing.cwnd = perf.group_initial_cwnd;
  timing.ssthresh = perf.initial_ssthresh; /* Will be reduced if any packets are dropped anyway */
  timing.num_replies_expected = 0;
  timing.num_replies_received = 0;
  timing.num_updates = 0;
  gettimeofday(&timing.last_drop, NULL);

  initialize_timeout_info(&to);

  num_probes_active = 0;
  num_probes_sent = num_probes_sent_at_last_wait = 0;
}


/* Returns true if the os scan system says that sending is OK.*/
bool ScanStats::sendOK() const {
  if (num_probes_sent - num_probes_sent_at_last_wait >= 50)
    return false;

  if (timing.cwnd < num_probes_active + 0.5)
    return false;

  return true;
}


/******************************************************************************
 * Implementation of class HostOsScan                                         *
 ******************************************************************************/

static unsigned int gcd_n_uint(int nvals, unsigned int *val) {
  unsigned int a, b, c;

  if (!nvals)
    return 1;
  a = *val;
  for (nvals--; nvals; nvals--) {
    b = *++val;
    if (a < b) {
      c = a;
      a = b;
      b = c;
    }
    while (b) {
      c = a % b;
      a = b;
      b = c;
    }
  }
  return a;
}

void HostOsScan::makeTSeqFP(HostOsScanStats *hss) {
  int i, j;
  u32 seq_diffs[NUM_SEQ_SAMPLES];
  u32 ts_diffs[NUM_SEQ_SAMPLES];
  float seq_rates[NUM_SEQ_SAMPLES];
  unsigned long time_usec_diffs[NUM_SEQ_SAMPLES];
  double seq_stddev = 0;
  double seq_rate = 0;
  double seq_avg_rate = 0;
  double avg_ts_hz = 0.0; /* Avg. amount that timestamps incr. each second */
  u32 seq_gcd = 1;
  int tcp_ipid_seqclass; /* TCP IPID SEQ TYPE defines in nmap.h */
  int tcp_closed_ipid_seqclass; /* TCP IPID SEQ TYPE defines in nmap.h */
  int icmp_ipid_seqclass; /* ICMP IPID SEQ TYPE defines in nmap.h */
  int good_tcp_ipid_num, good_tcp_closed_ipid_num, good_icmp_ipid_num;
  int tsnewval = 0;

  hss->FP_TSeq = new FingerTest(FingerPrintDef::SEQ, *o.reference_FPs->MatchPoints);
  FingerTest &test = *hss->FP_TSeq;

  /* Now we make sure there are no gaps in our response array ... */
  for (i = 0, j = 0; i < NUM_SEQ_SAMPLES; i++) {
    if (hss->si.seqs[i] != 0) /* We found a good one */ {
      if (j < i) {
        hss->si.seqs[j] = hss->si.seqs[i];
        hss->si.ipids[j] = hss->si.ipids[i];
        hss->si.timestamps[j] = hss->si.timestamps[i];
        hss->seq_send_times[j] = hss->seq_send_times[i];
      }
      if (j > 0) {
        seq_diffs[j - 1] = MOD_DIFF(hss->si.seqs[j], hss->si.seqs[j - 1]);

        ts_diffs[j - 1] = MOD_DIFF(hss->si.timestamps[j], hss->si.timestamps[j - 1]);
        time_usec_diffs[j - 1] = TIMEVAL_SUBTRACT(hss->seq_send_times[j], hss->seq_send_times[j - 1]);
        if (!time_usec_diffs[j - 1]) time_usec_diffs[j - 1]++; /* We divide by this later */
    /* Rate of ISN increase per second */
    seq_rates[j - 1] = seq_diffs[j - 1] * 1000000.0 / time_usec_diffs[j - 1];
    seq_avg_rate += seq_rates[j - 1];
      }
      j++;
    } /* Otherwise nothing good in this slot to copy */
  }

  hss->si.responses = j; /* Just for assurance */

  /* Time to look at the TCP ISN predictability */
  if (hss->si.responses >= 4 && o.scan_delay <= 1000) {
    seq_avg_rate /= hss->si.responses - 1;
    seq_rate = seq_avg_rate;

    /* First calculate the GCD */
    seq_gcd = gcd_n_uint(hss->si.responses -1, seq_diffs);

    if (!seq_gcd) {
      /* Constant ISN */
      seq_rate = 0;
      seq_stddev = 0;
      hss->si.index = 0;
    } else {

      /* Finally we take a binary logarithm, multiply by 8, and round
       * to get the final result */
      seq_rate = log(seq_rate) / log(2.0);
      seq_rate = (unsigned int) (seq_rate * 8 + 0.5);

      /* Normally we don't divide by gcd in computing the rate stddev
       * because otherwise we'll get an artificially low value about
       * 1/32 of the time if the responses all happen to be even.  On
       * the other hand, if a system inherently uses a large gcd such
       * as 64,000, we want to get rid of it.  So as a compromise, we
       * divide by the gcd if it is at least 9 */
      int div_gcd = 1;
      if (seq_gcd > 9)
        div_gcd = seq_gcd;

      for (i = 0; i < hss->si.responses - 1; i++) {
        double rtmp = seq_rates[i] / div_gcd - seq_avg_rate / div_gcd;
        seq_stddev += rtmp * rtmp;
      }

      /* We divide by ((numelements in seq_diffs) - 1), which is
       * (si.responses - 2), because that gives a better approx of
       * std. dev when you're only looking at a subset of whole
       * population. */
      seq_stddev /= hss->si.responses - 2;

      /* Next we need to take the square root of this value */
      seq_stddev = sqrt(seq_stddev);

      /* Finally we take a binary logarithm, multiply by 8, and round
       * to get the final result */
      if (seq_stddev <= 1)
        hss->si.index = 0;
      else {
        seq_stddev = log(seq_stddev) / log(2.0);
        hss->si.index = (int) (seq_stddev * 8 + 0.5);
      }
    }

    test.setAVal(str_SP_2_SP_2653, hss->target->FPR->cp_hex(hss->si.index));
    test.setAVal(str_GCD_3_GCD_70376, hss->target->FPR->cp_hex(seq_gcd));
    test.setAVal(str_ISR_3_ISR_72808, hss->target->FPR->cp_hex((unsigned int) seq_rate));
  } else if (hss->si.responses > 0) {
    if (o.debugging)
      log_write(LOG_PLAIN, str_Insuf_91_te92n_574383439, hss->target->targetipstr(), hss->si.responses);
  }

  /* Now it is time to deal with IPIDs */
  good_tcp_ipid_num = 0;
  good_tcp_closed_ipid_num = 0;
  good_icmp_ipid_num = 0;

  for (i = 0; i < NUM_SEQ_SAMPLES; i++) {
    if (hss->ipid.tcp_ipids[i] != 0xffffffff) {
      if (good_tcp_ipid_num < i) {
        hss->ipid.tcp_ipids[good_tcp_ipid_num] = hss->ipid.tcp_ipids[i];
      }
      good_tcp_ipid_num++;
    }

    if (hss->ipid.tcp_closed_ipids[i] != 0xffffffff) {
      if (good_tcp_closed_ipid_num < i) {
        hss->ipid.tcp_closed_ipids[good_tcp_closed_ipid_num] = hss->ipid.tcp_closed_ipids[i];
      }
      good_tcp_closed_ipid_num++;
    }

    if (hss->ipid.icmp_ipids[i] != 0xffffffff) {
      if (good_icmp_ipid_num < i) {
        hss->ipid.icmp_ipids[good_icmp_ipid_num] = hss->ipid.icmp_ipids[i];
      }
      good_icmp_ipid_num++;
    }
  }

  if (good_tcp_ipid_num >= 3) {
    tcp_ipid_seqclass = get_ipid_sequence_16(good_tcp_ipid_num, hss->ipid.tcp_ipids, islocalhost(hss->target->TargetSockAddr()));
  } else {
    tcp_ipid_seqclass = IPID_SEQ_UNKNOWN;
  }
  /* Only print open tcp ipid seqclass in the final report. */
  hss->si.ipid_seqclass = tcp_ipid_seqclass;

  if (good_tcp_closed_ipid_num >= 2) {
    tcp_closed_ipid_seqclass = get_ipid_sequence_16(good_tcp_closed_ipid_num, hss->ipid.tcp_closed_ipids, islocalhost(hss->target->TargetSockAddr()));
  } else {
    tcp_closed_ipid_seqclass = IPID_SEQ_UNKNOWN;
  }

  if (good_icmp_ipid_num >= 2) {
    icmp_ipid_seqclass = get_ipid_sequence_16(good_icmp_ipid_num, hss->ipid.icmp_ipids, islocalhost(hss->target->TargetSockAddr()));
  } else {
    icmp_ipid_seqclass = IPID_SEQ_UNKNOWN;
  }

  /* This fills in TI=Z or something like that. */
  test.setAVal(str_TI_2_TI_2677, make_aval_ipid_seq(tcp_ipid_seqclass, hss->ipid.tcp_ipids, hss));
  test.setAVal(str_CI_2_CI_2150, make_aval_ipid_seq(tcp_closed_ipid_seqclass, hss->ipid.tcp_closed_ipids, hss));
  test.setAVal(str_II_2_II_2336, make_aval_ipid_seq(icmp_ipid_seqclass, hss->ipid.icmp_ipids, hss));

  /* SS: Shared IP ID sequence boolean */
  if ((tcp_ipid_seqclass == IPID_SEQ_INCR ||
        tcp_ipid_seqclass == IPID_SEQ_BROKEN_INCR ||
        tcp_ipid_seqclass == IPID_SEQ_RPI) &&
       (icmp_ipid_seqclass == IPID_SEQ_INCR ||
        icmp_ipid_seqclass == IPID_SEQ_BROKEN_INCR ||
        icmp_ipid_seqclass == IPID_SEQ_RPI)) {
    /* Both are incremental. Thus we have str_SS_2_SS_2656 test. Check if they
       are in the same sequence. */
    u32 avg = (hss->ipid.tcp_ipids[good_tcp_ipid_num - 1] - hss->ipid.tcp_ipids[0]) / (good_tcp_ipid_num - 1);
    if (hss->ipid.icmp_ipids[0] < hss->ipid.tcp_ipids[good_tcp_ipid_num - 1] + 3 * avg) {
      test.setAVal(str_SS_2_SS_2656, str_S_1_S_83);
    } else {
      test.setAVal("SS", str_O_1_O_79);
    }
  }

  /* Now we look at TCP Timestamp sequence prediction */
  /* Battle plan:
     1) Compute average increments per second, and variance in incr. per second
     2) If any are 0, set to constant
     3) If variance is high, set to random incr. [ skip for now ]
     4) if ~10/second, set to appropriate thing
     5) Same with ~100/sec
  */
  if (hss->si.ts_seqclass == TS_SEQ_UNKNOWN && hss->si.responses >= 2) {
    time_t uptime = 0;
    avg_ts_hz = 0.0;
    for (i = 0; i < hss->si.responses - 1; i++) {
      double dhz;

      dhz = (double) ts_diffs[i] / (time_usec_diffs[i] / 1000000.0);
      /*       printf("ts incremented by %d in %li usec -- %fHZ\n", ts_diffs[i], time_usec_diffs[i], dhz); */
      avg_ts_hz += dhz / (hss->si.responses - 1);
    }

    if (avg_ts_hz > 0 && avg_ts_hz < 5.66) { /* relatively wide range because sampling time so short and frequency so slow */
      hss->si.ts_seqclass = TS_SEQ_2HZ;
      uptime = hss->si.timestamps[0] / 2;
    }
    else if (avg_ts_hz > 70 && avg_ts_hz < 150) {
      hss->si.ts_seqclass = TS_SEQ_100HZ;
      uptime = hss->si.timestamps[0] / 100;
    }
    else if (avg_ts_hz > 724 && avg_ts_hz < 1448) {
      hss->si.ts_seqclass = TS_SEQ_1000HZ;
      uptime = hss->si.timestamps[0] / 1000;
    }
    else if (avg_ts_hz > 0) {
      hss->si.ts_seqclass = TS_SEQ_OTHER_NUM;
      uptime = hss->si.timestamps[0] / (unsigned int)(0.5 + avg_ts_hz);
    }

    if (uptime > 63072000) {
      /* Up 2 years?  Perhaps, but they're probably lying. */
      if (o.debugging) {
        /* long long is probably excessive for number of days, but sick of
         * truncation warnings and finding the right format string for time_t
         */
        log_write(LOG_STDOUT, str_Ignor_41_ys92n_571287998,
        hss->target->targetipstr(), (long long) (uptime / 86400));
      }
      uptime = 0;
    }
    hss->si.lastboot = hss->seq_send_times[0].tv_sec - uptime;
  }

  switch (hss->si.ts_seqclass) {

  case TS_SEQ_ZERO:
    test.setAVal(str_TS_2_TS_2687, str_0_1_0_48);
    break;
  case TS_SEQ_2HZ:
  case TS_SEQ_100HZ:
  case TS_SEQ_1000HZ:
  case TS_SEQ_OTHER_NUM:
    /* Here we "cheat" a little to make the classes correspond more
       closely to common real-life frequencies (particularly 100)
       which aren't powers of two. */
    if (avg_ts_hz <= 5.66) {
      /* 1 would normally range from 1.4 - 2.82, but we expand that
         to 0 - 5.66, so we won't ever even get a value of 2.  Needs
         to be wide because our test is so fast that it is hard to
         match slow frequencies exactly.  */
      tsnewval = 1;
    } else if (avg_ts_hz > 70 && avg_ts_hz <= 150) {
      /* mathematically 7 would be 90.51 - 181, but we change to 70-150 to
         better align with common freq 100 */
      tsnewval = 7;
    } else if (avg_ts_hz > 150 && avg_ts_hz <= 350) {
      /* would normally be 181 - 362.  Now aligns better with 200 */
      tsnewval = 8;
    } else {
      /* Do a log base2 rounded to nearest int */
      tsnewval = (unsigned int)(0.5 + log(avg_ts_hz) / log(2.0));
    }

    test.setAVal(str_TS_2_TS_2687, hss->target->FPR->cp_hex(tsnewval));
    break;
  case TS_SEQ_UNSUPPORTED:
    test.setAVal(str_TS_2_TS_2687, str_U_1_U_85);
    break;
  }
}


void HostOsScan::makeTOpsFP(HostOsScanStats *hss) {
  assert(hss);
  int i, n;

  if (hss->TOpsReplyNum != 6)
    return;

  for (n = 0; n < 6; n++) {
    if (!hss->TOps_AVs[n])
      break;
  }
  if (n < 6) {
    if (o.debugging)
      error(str_We32d_42_3237s_380390192, hss->target->targetipstr());
    return;
  }

  hss->FP_TOps = new FingerTest(FingerPrintDef::OPS, *o.reference_FPs->MatchPoints);
  std::vector<const char *> &results = *hss->FP_TOps->results;

  for (i = 0; i < n; i++)
    results[i] = hss->TOps_AVs[i];
}


void HostOsScan::makeTWinFP(HostOsScanStats *hss) {
  assert(hss);
  int i, n;

  if (hss->TWinReplyNum != 6)
    return;

  for (n = 0; n < 6; n++) {
    if (!hss->TWin_AVs[n])
      break;
  }
  if (n < 6) {
    if (o.debugging)
      error(str_We32d_42_3237s_1312528634, hss->target->targetipstr());
    return;
  }

  hss->FP_TWin = new FingerTest(FingerPrintDef::WIN, *o.reference_FPs->MatchPoints);
  std::vector<const char *> &results = *hss->FP_TWin->results;

  for (i = 0; i < n; i++)
    results[i] = hss->TWin_AVs[i];
}


bool HostOsScan::processTSeqResp(HostOsScanStats *hss, const struct ip *ip, int replyNo) {
  assert(replyNo >= 0 && replyNo < NUM_SEQ_SAMPLES);

  const struct tcp_hdr *tcp;
  int seq_response_num; /* response # for sequencing */
  u32 timestamp = 0; /* TCP timestamp we receive back */

  if (hss->lastipid != 0 && ip->ip_id == hss->lastipid) {
    /* Probably a duplicate -- this happens sometimes when scanning localhost */
    return false;
  }
  hss->lastipid = ip->ip_id;

  tcp = ((struct tcp_hdr *) (((char *) ip) + 4 * ip->ip_hl));

  if ((tcp->th_flags & TH_RST)) {
    if (hss->si.responses == 0) {
      error(str_WARNI_57_pen63_2068248250,
              hss->target->targetipstr(), hss->openTCPPort);
    }
    return false;
  }

  if ((tcp->th_flags & (TH_SYN|TH_ACK)) == (TH_SYN|TH_ACK)) {
    /*  error("DEBUG: response is SYN|ACK to port %hu\n", ntohs(tcp->th_dport)); */
    /*readtcppacket((char *)ip, ntohs(ip->ip_len));*/
    /* We use the ACK value to match up our sent with rcv'd packets */
    seq_response_num = ntohl(tcp->th_ack) - tcpSeqBase - 1;
    /* printf("seq_response_num = %d\treplyNo = %d\n", seq_response_num, replyNo); */

    if (seq_response_num != replyNo) {
      /* BzzT! Value out of range */
      if (o.debugging) {
        error(str_Unabl_61_37s46_649452452,
              hss->target->targetipstr());
        error(str_Recei_46_ket58_2072200780,
              (unsigned long) ntohl(tcp->th_ack),
              (unsigned long) tcpSeqBase);
        readtcppacket((unsigned char *)ip, ntohs(ip->ip_len));
      }
      seq_response_num = replyNo;
    }

    if (hss->si.seqs[seq_response_num] == 0) {
      /* New response found! */
      hss->si.responses++;
      hss->si.seqs[seq_response_num] = ntohl(tcp->th_seq); /* TCP ISN */
      hss->si.ipids[seq_response_num] = ntohs(ip->ip_id);

      if ((gettcpopt_ts(tcp, &timestamp, NULL) == 0))
        hss->si.ts_seqclass = TS_SEQ_UNSUPPORTED;
      else {
        if (timestamp == 0) {
          hss->si.ts_seqclass = TS_SEQ_ZERO;
        }
      }
      hss->si.timestamps[seq_response_num] = timestamp;
      /* printf("Response #%d -- ipid=%hu ts=%i\n", seq_response_num, ntohs(ip->ip_id), timestamp); */

      return true;
    }
  }

  return false;
}


bool HostOsScan::processTOpsResp(HostOsScanStats *hss, const struct tcp_hdr *tcp, int replyNo) {
  assert(replyNo >= 0 && replyNo < 6);
  char ops_buf[256];

  if (hss->FP_TOps || hss->TOps_AVs[replyNo])
    return false;

  int opsParseResult = get_tcpopt_string(tcp, this->tcpMss, ops_buf, sizeof(ops_buf));

  if (opsParseResult <= 0) {
    if (opsParseResult < 0 && o.debugging)
      error(str_Optio_48_37s46_2098152421, replyNo, hss->target->targetipstr());
    hss->TOps_AVs[replyNo] = "";
  }
  else {
    hss->TOps_AVs[replyNo] = hss->target->FPR->cp_dup(ops_buf, opsParseResult);
  }

  hss->TOpsReplyNum++;
  return true;
}


bool HostOsScan::processTWinResp(HostOsScanStats *hss, const struct tcp_hdr *tcp, int replyNo) {
  assert(replyNo >= 0 && replyNo < 6);

  if (hss->FP_TWin || hss->TWin_AVs[replyNo])
    return false;

  hss->TWin_AVs[replyNo] = hss->target->FPR->cp_hex(ntohs(tcp->th_win));

  hss->TWinReplyNum++;
  return true;
}


bool HostOsScan::processTEcnResp(HostOsScanStats *hss, const struct ip *ip) {
  char ops_buf[256];
  char quirks_buf[10];
  char *p;
  const struct tcp_hdr *tcp = ((struct tcp_hdr *) (((char *) ip) + 4 * ip->ip_hl));

  if (hss->FP_TEcn)
    return false;

  /* Create the Avals */
  hss->FP_TEcn = new FingerTest(FingerPrintDef::ECN, *o.reference_FPs->MatchPoints);
  FingerTest &test = *hss->FP_TEcn;

  test.setAVal(str_R_1_R_82, str_Y_1_Y_89);

  /* don't frag flag */
  if (ntohs(ip->ip_off) & IP_DF)
    test.setAVal(str_DF_2_DF_2178, str_Y_1_Y_89);
  else
    test.setAVal(str_DF_2_DF_2178, str_N_1_N_78);

  /* TTL */
  test.setAVal(str_T_1_T_84, hss->target->FPR->cp_hex(ip->ip_ttl));

  /* TCP Window size */
  test.setAVal(str_W_1_W_87, hss->target->FPR->cp_hex(ntohs(tcp->th_win)));

  /* Now for the TCP options ... */
  int opsParseResult = get_tcpopt_string(tcp, this->tcpMss, ops_buf, sizeof(ops_buf));

  if (opsParseResult <= 0) {
    if (opsParseResult < 0 && o.debugging)
      error(str_Optio_44_37s46_1931725784, hss->target->targetipstr());
    test.setAVal(str_O_1_O_79, "");
  }
  else {
    test.setAVal(str_O_1_O_79, hss->target->FPR->cp_dup(ops_buf, opsParseResult));
  }

  /* Explicit Congestion Notification support test */
  if ((tcp->th_flags & TH_ECE) && (tcp->th_flags & TH_CWR))
    /* echo back */
    test.setAVal(str_CC_2_CC_2144, str_S_1_S_83);
  else if (tcp->th_flags & TH_ECE)
    /* support */
    test.setAVal(str_CC_2_CC_2144, str_Y_1_Y_89);
  else if (!(tcp->th_flags & TH_CWR))
    /* not support */
    test.setAVal(str_CC_2_CC_2144, str_N_1_N_78);
  else
    test.setAVal(str_CC_2_CC_2144, str_O_1_O_79);

  /* TCP miscellaneous quirks test */
  p = quirks_buf;
  if (tcp->th_x2) {
    /* Reserved field of TCP is not zero */
    assert(p + 1 < quirks_buf + sizeof(quirks_buf));
    *p++ = 'R';
  }
  if (!(tcp->th_flags & TH_URG) && tcp->th_urp) {
    /* URG pointer value when urg flag not set */
    assert(p + 1 < quirks_buf + sizeof(quirks_buf));
    *p++ = 'U';
  }
  *p = '\0';
  test.setAVal(str_Q_1_Q_81, hss->target->FPR->cp_dup(quirks_buf, p - quirks_buf));

  return true;
}


bool HostOsScan::processT1_7Resp(HostOsScanStats *hss, const struct ip *ip, int replyNo) {
  assert(replyNo >= 0 && replyNo < 7);

  const struct tcp_hdr *tcp = ((struct tcp_hdr *) (((char *) ip) + 4 * ip->ip_hl));

  int i;
  int length;
  char flags_buf[10];
  char quirks_buf[10];
  char *p;
  FingerPrintDef::TestID testid = INT2ID(ID2INT(FingerPrintDef::T1) + replyNo);

  if (hss->FPtests[FP_T1_7_OFF + replyNo])
    return false;

  hss->FPtests[FP_T1_7_OFF + replyNo] = new FingerTest(testid, *o.reference_FPs->MatchPoints);
  FingerTest &test = *hss->FPtests[FP_T1_7_OFF + replyNo];

  /* First we give the "response" flag to say we did actually receive
     a packet -- this way we won't match a template with R=N */
  test.setAVal(str_R_1_R_82, str_Y_1_Y_89);

  /* Next we check whether the Don't Fragment bit is set */
  if (ntohs(ip->ip_off) & IP_DF)
    test.setAVal(str_DF_2_DF_2178, str_Y_1_Y_89);
  else
    test.setAVal(str_DF_2_DF_2178, str_N_1_N_78);

  /* TTL */
  test.setAVal(str_T_1_T_84, hss->target->FPR->cp_hex(ip->ip_ttl));

  if (replyNo != 0) {
    /* Now we do the TCP Window size */
    test.setAVal(str_W_1_W_87, hss->target->FPR->cp_hex(ntohs(tcp->th_win)));
  }

  /* Seq test values:
     Z   = zero
     A   = same as ack
     A+  = ack + 1
     O   = other
  */
  if (ntohl(tcp->th_seq) == 0)
    test.setAVal(str_S_1_S_83, str_Z_1_Z_90);
  else if (ntohl(tcp->th_seq) == tcpAck)
    test.setAVal(str_S_1_S_83, str_A_1_A_65);
  else if (ntohl(tcp->th_seq) == tcpAck + 1)
    test.setAVal(str_S_1_S_83, str_A43_2_A43_64128);
  else
    test.setAVal(str_S_1_S_83, str_O_1_O_79);

  /* ACK test values:
     Z   = zero
     S   = same as syn
     S+  = syn + 1
     O   = other
  */
  if (ntohl(tcp->th_ack) == 0)
    test.setAVal(str_A_1_A_65, str_Z_1_Z_90);
  else if (ntohl(tcp->th_ack) == tcpSeqBase)
    test.setAVal(str_A_1_A_65, str_S_1_S_83);
  else if (ntohl(tcp->th_ack) == tcpSeqBase + 1)
    test.setAVal(str_A_1_A_65, str_S43_2_S43_81426);
  else
    test.setAVal(str_A_1_A_65, str_O_1_O_79);

  /* Flags. They must be in this order:
     E = ECN Echo
     U = Urgent
     A = Acknowledgement
     P = Push
     R = Reset
     S = Synchronize
     F = Final
  */
  struct {
    u8 flag;
    char c;
  } flag_defs[] = {
    { TH_ECE, 'E' },
    { TH_URG, 'U' },
    { TH_ACK, 'A' },
    { TH_PUSH, 'P' },
    { TH_RST, 'R' },
    { TH_SYN, 'S' },
    { TH_FIN, 'F' },
  };
  assert(sizeof(flag_defs) / sizeof(flag_defs[0]) < sizeof(flags_buf));
  p = flags_buf;
  for (i = 0; i < (int) (sizeof(flag_defs) / sizeof(flag_defs[0])); i++) {
    if (tcp->th_flags & flag_defs[i].flag)
      *p++ = flag_defs[i].c;
  }
  *p = '\0';
  test.setAVal(str_F_1_F_70, hss->target->FPR->cp_dup(flags_buf, p - flags_buf));

  if (replyNo != 0) {
    char ops_buf[256];

    /* Now for the TCP options ... */
    int opsParseResult = get_tcpopt_string(tcp, this->tcpMss, ops_buf, sizeof(ops_buf));
    if (opsParseResult <= 0) {
      if (opsParseResult < 0 && o.debugging)
        error(str_Optio_44_37s46_1876126590, replyNo, hss->target->targetipstr());
      test.setAVal(str_O_1_O_79, "");
    }
    else {
      test.setAVal(str_O_1_O_79, hss->target->FPR->cp_dup(ops_buf, opsParseResult));
    }
  }

  /* Rst Data CRC32 */
  length = (int) ntohs(ip->ip_len) - 4 * ip->ip_hl -4 * tcp->th_off;
  if ((tcp->th_flags & TH_RST) && length>0) {
    test.setAVal(str_RD_2_RD_2610, hss->target->FPR->cp_hex(nbase_crc32(((u8 *)tcp) + 4 * tcp->th_off, length)));
  } else {
    test.setAVal(str_RD_2_RD_2610, str_0_1_0_48);
  }

  /* TCP miscellaneous quirks test */
  p = quirks_buf;
  if (tcp->th_x2) {
    /* Reserved field of TCP is not zero */
    assert(p + 1 < quirks_buf + sizeof(quirks_buf));
    *p++ = 'R';
  }
  if (!(tcp->th_flags & TH_URG) && tcp->th_urp) {
    /* URG pointer value when urg flag not set */
    assert(p + 1 < quirks_buf + sizeof(quirks_buf));
    *p++ = 'U';
  }
  *p = '\0';
  test.setAVal(str_Q_1_Q_81, hss->target->FPR->cp_dup(quirks_buf, p - quirks_buf));

  return true;
}


bool HostOsScan::processTUdpResp(HostOsScanStats *hss, const struct ip *ip) {

  assert(hss);
  assert(ip);

  const struct icmp *icmp;
  const struct ip *ip2;
  unsigned short checksum;
  unsigned short *checksumptr;
  const struct udp_hdr *udp;
  const unsigned char *datastart, *dataend;

  if (hss->FP_TUdp)
    return false;

  icmp = ((struct icmp *)(((char *) ip) + 4 * ip->ip_hl));

  /* Make sure this is icmp port unreachable. */
  assert(icmp->icmp_type == 3 && icmp->icmp_code == 3);

  ip2 = (struct ip*)((char *)icmp + 8);
  udp = (struct udp_hdr *)((char *)ip2 + 4 * ip2->ip_hl);

  /* The ports should match. */
  if (ntohs(udp->uh_sport) != hss->upi.sport || ntohs(udp->uh_dport) != hss->upi.dport) {
    return false;
  }

  hss->FP_TUdp = new FingerTest(FingerPrintDef::U1, *o.reference_FPs->MatchPoints);
  FingerTest &test = *hss->FP_TUdp;

  /* First of all, if we got this far the response was yes */
  test.setAVal(str_R_1_R_82, str_Y_1_Y_89);

  /* Also, we now know that the port we reached was closed */
  if (hss->target->FPR->osscan_closedudpport == -1)
    hss->target->FPR->osscan_closedudpport = hss->upi.dport;

  /* Now let us do an easy one, Don't fragment */
  if (ntohs(ip->ip_off) & IP_DF)
    test.setAVal(str_DF_2_DF_2178, str_Y_1_Y_89);
  else
    test.setAVal(str_DF_2_DF_2178, str_N_1_N_78);

  /* TTL */
  test.setAVal(str_T_1_T_84, hss->target->FPR->cp_hex(ip->ip_ttl));

  /* Now we look at the IP datagram length that was returned, some
     machines send more of the original packet back than others */
  test.setAVal(str_IPL_3_IPL_72709, hss->target->FPR->cp_hex(ntohs(ip->ip_len)));

  /* unused filed not zero in Destination Unreachable Message */
  test.setAVal(str_UN_2_UN_2713, hss->target->FPR->cp_hex(ntohl(icmp->icmp_void)));

  /* OK, lets check the returned IP length, some systems @$@ this
     up */
  if (ntohs(ip2->ip_len) == 328)
    test.setAVal(str_RIPL_4_RIPL_2515571, str_G_1_G_71);
  else
    test.setAVal(str_RIPL_4_RIPL_2515571, hss->target->FPR->cp_hex(ntohs(ip2->ip_len)));

  /* This next test doesn't work on Solaris because the lamers
     overwrite our ip_id */
#if !defined(SOLARIS) && !defined(SUNOS) && !defined(IRIX) && !defined(HPUX)

  /* Now lets see how they treated the ID we sent ... */
  if (ntohs(ip2->ip_id) == hss->upi.ipid)
    test.setAVal(str_RID_3_RID_81133, str_G_1_G_71); /* The good "expected" value */
  else
    test.setAVal(str_RID_3_RID_81133, hss->target->FPR->cp_hex(ntohs(ip2->ip_id)));

#endif

  /* Let us see if the IP checksum we got back computes */

  /* Thanks to some machines not having struct ip member ip_sum we
     have to go with this BS */
  checksumptr = (unsigned short *)   ((char *) ip2 + 10);
  checksum = *checksumptr;

  if (checksum == 0) {
    test.setAVal(str_RIPCK_5_RIPCK_77982497, str_Z_1_Z_90);
  } else {
    *checksumptr = 0;
    if (in_cksum((unsigned short *)ip2, 20) == checksum) {
      test.setAVal(str_RIPCK_5_RIPCK_77982497, str_G_1_G_71); /* The "expected" good value */
    } else {
      test.setAVal(str_RIPCK_5_RIPCK_77982497, str_I_1_I_73); /* They modified it */
    }
    *checksumptr = checksum;
  }

  /* UDP checksum */
  if (udp->uh_sum == hss->upi.udpck)
    test.setAVal(str_RUCK_4_RUCK_2526699, str_G_1_G_71); /* The "expected" good value */
  else
    test.setAVal(str_RUCK_4_RUCK_2526699, hss->target->FPR->cp_hex(ntohs(udp->uh_sum)));

  /* Finally we ensure the data is OK */
  datastart = ((unsigned char *)udp) + 8;
  dataend = (unsigned char *)  ip + ntohs(ip->ip_len);

  while (datastart < dataend) {
    if (*datastart != hss->upi.patternbyte)
      break;
    datastart++;
  }
  if (datastart < dataend)
    test.setAVal(str_RUD_3_RUD_81505, str_I_1_I_73); /* They modified it */
  else
    test.setAVal(str_RUD_3_RUD_81505, str_G_1_G_71);

  /* Count hop count */
  if (hss->distance == -1) {
    hss->distance = this->udpttl - ip2->ip_ttl + 1;
  }

  return true;
}


bool HostOsScan::processTIcmpResp(HostOsScanStats *hss, const struct ip *ip, int replyNo) {
  assert(replyNo == 0 || replyNo == 1);

  const struct ip *ip1, *ip2;
  const struct icmp *icmp1, *icmp2;
  unsigned short value1, value2;

  if (hss->FP_TIcmp)
    return false;

  if (hss->icmpEchoReply == NULL) {
    /* This is the first icmp reply we get, store it and return. */
    hss->icmpEchoReply = (struct ip *) safe_malloc(ntohs(ip->ip_len));
    memcpy(hss->icmpEchoReply, ip, ntohs(ip->ip_len));
    hss->storedIcmpReply = replyNo;
    return true;
  } else if (hss->storedIcmpReply == replyNo) {
    /* This is a duplicated icmp reply. */
    return false;
  }

  /* Ok, now we get another reply. */
  if (hss->storedIcmpReply == 0) {
    ip1 = hss->icmpEchoReply;
    ip2 = ip;
  } else {
    ip1 = ip;
    ip2 = hss->icmpEchoReply;
  }

  icmp1 = ((struct icmp *)(((char *) ip1) + 4 * ip1->ip_hl));
  icmp2 = ((struct icmp *)(((char *) ip2) + 4 * ip2->ip_hl));

  assert(icmp1->icmp_type == 0 && icmp2->icmp_type == 0);

  hss->FP_TIcmp= new FingerTest(FingerPrintDef::IE, *o.reference_FPs->MatchPoints);
  FingerTest &test = *hss->FP_TIcmp;

  test.setAVal(str_R_1_R_82, str_Y_1_Y_89);

  /* DFI test values:
   * Y. Both set DF;
   * S. Both use the DF that the sender uses;
   * N. Both not set;
   * O. Other(both different with the sender, -_-b).
   */
  value1 = (ntohs(ip1->ip_off) & IP_DF);
  value2 = (ntohs(ip2->ip_off) & IP_DF);
  if (value1 && value2)
    /* both set */
    test.setAVal(str_DFI_3_DFI_67591, str_Y_1_Y_89);
  else if (value1 && !value2)
    /* echo back */
    test.setAVal(str_DFI_3_DFI_67591, str_S_1_S_83);
  else if (!value1 && !value2)
    /* neither set */
    test.setAVal(str_DFI_3_DFI_67591, str_N_1_N_78);
  else
    test.setAVal(str_DFI_3_DFI_67591, str_O_1_O_79);

  /* TTL */

  test.setAVal(str_T_1_T_84, hss->target->FPR->cp_hex(ip1->ip_ttl));

  /* ICMP Code value. Test values:
   * [Value]. Both set Code to the same value [Value];
   * S. Both use the Code that the sender uses;
   * O. Other.
   */
  value1 = icmp1->icmp_code;
  value2 = icmp2->icmp_code;
  if (value1 == value2) {
    if (value1 == 0)
      test.setAVal(str_CD_2_CD_2145, str_Z_1_Z_90);
    else
      test.setAVal(str_CD_2_CD_2145, hss->target->FPR->cp_hex(value1));
  }
  else if (value1 == 9 && value2 == 0)
    /* both the same as in the corresponding probe */
    test.setAVal(str_CD_2_CD_2145, str_S_1_S_83);
  else
    test.setAVal(str_CD_2_CD_2145, str_O_1_O_79);

  return true;
}


int HostOsScan::get_tcpopt_string(const struct tcp_hdr *tcp, int mss, char *result, int maxlen) const {
  char *p;
  const char *q;
  u16 tmpshort;
  u32 tmpword;
  int length;
  int opcode;

  p = result;
  length = (tcp->th_off * 4) - sizeof(struct tcp_hdr);
  q = ((char *)tcp) + sizeof(struct tcp_hdr);

  /*
   * Example parsed result: M5B4ST11NW2
   *   MSS, Sack Permitted, Timestamp with both value not zero, Nop, WScale with value 2
   */

  /* Be aware of the max increment value for p in parsing,
   * now is 5 = strlen("Mxxxx") <-> MSS Option
   */
  while (length > 0 && (p - result) < (maxlen - 5)) {
    opcode = *q++;
    if (!opcode) { /* End of List */
      *p++ = 'L';
      length--;
    } else if (opcode == 1) { /* No Op */
      *p++ = 'N';
      length--;
    } else if (opcode == 2) { /* MSS */
      if (length < 4)
        break; /* MSS has 4 bytes */
      *p++ = 'M';
      q++;
      memcpy(&tmpshort, q, 2);
      /*  if (ntohs(tmpshort) == mss) */
      /*    *p++ = 'E'; */
      sprintf(p, str_37hX_3_37hX_1575508, ntohs(tmpshort));
      p += strlen(p); /* max movement of p is 4 (0xFFFF) */
      q += 2;
      length -= 4;
    } else if (opcode == 3) { /* Window Scale */
      if (length < 3)
        break; /* Window Scale option has 3 bytes */
      *p++ = 'W';
      q++;
      snprintf(p, length, str_37hhX_4_37hhX_48841332, *((u8*)q));
      p += strlen(p); /* max movement of p is 2 (max WScale value is 0xFF) */
      q++;
      length -= 3;
    } else if (opcode == 4) { /* SACK permitted */
      if (length < 2)
        break; /* SACK permitted option has 2 bytes */
      *p++ = 'S';
      q++;
      length -= 2;
    } else if (opcode == 8) { /* Timestamp */
      if (length < 10)
        break; /* Timestamp option has 10 bytes */
      *p++ = 'T';
      q++;
      memcpy(&tmpword, q, 4);
      if (tmpword)
        *p++ = '1';
      else
        *p++ = '0';
      q += 4;
      memcpy(&tmpword, q, 4);
      if (tmpword)
        *p++ = '1';
      else
        *p++ = '0';
      q += 4;
      length -= 10;
    }
  }

  if (length > 0) {
    /* We could reach here for one of the two reasons:
     *  1. At least one option is not correct. (Eg. Should have 4 bytes but only has 3 bytes left).
     *  2. The option string is too long.
     */
    *result = '\0';
    return -1;
  }

  *p = '\0';
  return p - result;
}


/******************************************************************************
 * Implementation of class HostOsScanInfo                                     *
 ******************************************************************************/

HostOsScanInfo::HostOsScanInfo(Target *t, OsScanInfo *OsSI) {
  target = t;
  OSI = OsSI;

  FPs = (FingerPrint **) safe_zalloc(o.maxOSTries() * sizeof(FingerPrint *));
  FP_matches = new FingerPrintResultsIPv4[o.maxOSTries()];
  timedOut = false;
  isCompleted = false;

  if (target->FPR == NULL) {
    this->FPR = new FingerPrintResultsIPv4;
    target->FPR = this->FPR;
  }
  target->osscanSetFlag(OS_PERF);

  hss = new HostOsScanStats(t);
}


HostOsScanInfo::~HostOsScanInfo() {
  delete hss;
  free(FPs);
  delete[] FP_matches;
}


/******************************************************************************
 * Implementation of class OsScanInfo                                         *
 ******************************************************************************/

OsScanInfo::OsScanInfo(std::vector<Target *> &Targets) {
  unsigned int targetno;
  HostOsScanInfo *hsi;
  int num_timedout = 0;

  gettimeofday(&now, NULL);

  numInitialTargets = 0;

  /* build up incompleteHosts list */
  for (targetno = 0; targetno < Targets.size(); targetno++) {
    /* check if Targets[targetno] is good to be scanned
     * if yes, append it to the list
     */
    if (Targets[targetno]->timedOut(&now)) {
      num_timedout++;
      continue;
    }

#ifdef WIN32
    if (g_has_npcap_loopback == 0 && Targets[targetno]->ifType() == devt_loopback) {
      log_write(LOG_STDOUT, str_Skipp_90_4192n_170441350, Targets[targetno]->NameIP());
      continue;
    }
#endif

    if (Targets[targetno]->ports.getStateCounts(IPPROTO_TCP, PORT_OPEN) == 0 ||
        (Targets[targetno]->ports.getStateCounts(IPPROTO_TCP, PORT_CLOSED) == 0 &&
         Targets[targetno]->ports.getStateCounts(IPPROTO_TCP, PORT_UNFILTERED) == 0)) {
      if (o.osscan_limit) {
        if (o.verbose)
          log_write(LOG_PLAIN, str_Skipp_78_ts92n_1145675994, Targets[targetno]->NameIP());
        continue;
      } else {
        Targets[targetno]->osscanSetFlag(OS_PERF_UNREL);
      }
    }

    hsi = new HostOsScanInfo(Targets[targetno], this);
    incompleteHosts.push_back(hsi);
    numInitialTargets++;
  }

  nextI = incompleteHosts.begin();
}


OsScanInfo::~OsScanInfo()
{
  while (!incompleteHosts.empty()) {
    delete incompleteHosts.front();
    incompleteHosts.pop_front();
  }
}


/* Find a HostScanStats by IP its address in the incomplete list.  Returns NULL if
   none are found. */
HostOsScanInfo *OsScanInfo::findIncompleteHost(const struct sockaddr_storage *ss) {
  std::list<HostOsScanInfo *>::iterator hostI;
  const struct sockaddr_in *sin = (struct sockaddr_in *) ss;

  if (sin->sin_family != AF_INET)
    fatal(str_37s32_28_dress_297803097, __func__);

  for (hostI = incompleteHosts.begin(); hostI != incompleteHosts.end(); hostI++) {
    if ((*hostI)->target->v4hostip()->s_addr == sin->sin_addr.s_addr)
      return *hostI;
  }
  return NULL;
}


/* A circular buffer of the incompleteHosts.  nextIncompleteHost() gives
   the next one.  The first time it is called, it will give the
   first host in the list.  If incompleteHosts is empty, returns
   NULL. */
HostOsScanInfo *OsScanInfo::nextIncompleteHost() {
  HostOsScanInfo *nxt;

  if (incompleteHosts.empty())
    return NULL;

  nxt = *nextI;
  nextI++;
  if (nextI == incompleteHosts.end())
    nextI = incompleteHosts.begin();

  return nxt;
}


/* Removes any hosts that have completed their scans from the incompleteHosts
   list.  Returns the number of hosts removed. */
int OsScanInfo::removeCompletedHosts() {
  std::list<HostOsScanInfo *>::iterator hostI, nxt;
  HostOsScanInfo *hsi = NULL;
  int hostsRemoved = 0;
  bool timedout = false;

  for (hostI = incompleteHosts.begin(); hostI != incompleteHosts.end();
      hostI = nxt) {
    nxt = hostI;
    nxt++;
    hsi = *hostI;
    timedout = hsi->target->timedOut(&now);
    if (hsi->isCompleted || timedout) {
      /* A host to remove!  First adjust nextI appropriately */
      if (nextI == hostI && incompleteHosts.size() > 1) {
        nextI++;
        if (nextI == incompleteHosts.end())
          nextI = incompleteHosts.begin();
      }

      if (o.verbose && numInitialTargets > 50) {
        int remain = incompleteHosts.size() - 1;
        if (remain && !timedout)
          log_write(LOG_STDOUT, str_Compl_47_4192n_1106110510,
                    hsi->target->targetipstr(),
                    o.TimeSinceStart() - this->starttime, remain,
                    (remain == 1)? str_host3_9_2left_2080999794 : str_hosts_10_2left_854448753);
        else if (timedout)
          log_write(LOG_STDOUT, str_37s32_37_4192n_613345288,
                    hsi->target->targetipstr(), remain,
                    (remain == 1)? str_host3_9_2left_2080999794 : str_hosts_10_2left_854448753);
      }
      incompleteHosts.erase(hostI);
      hostsRemoved++;
      hsi->target->stopTimeOutClock(&now);
      delete hsi;
    }
  }
  return hostsRemoved;
}

/******************************************************************************
 * Implementation of class OSScan()                                           *
 ******************************************************************************/

/* Constructor */
OSScan::OSScan() {
  this->reset();
  return;
}

/* Destructor */
OSScan::~OSScan() {
  return;
}

/* Function that initializes internal variables */
void OSScan::reset() {

}


/* This function takes a group of targets and divides it in chunks if there are
 * too many to be processed at the same time. The threshold is based on Nmap's
 * timing level (when timing level is above 4, no chunking is performed).
 * The reason targets are processed in smaller groups is to improve accuracy. */
int OSScan::chunk_and_do_scan(std::vector<Target *> &Targets, int family) {
  unsigned int max_os_group_sz = 20;
  double fudgeratio = 1.2; /* Allow a slightly larger final group rather than finish with a tiny one */
  std::vector<Target *> tmpTargets;
  unsigned int startidx = 0;

  if (o.timing_level == 4)
    max_os_group_sz = (unsigned int) (max_os_group_sz * 1.5);

  if (o.timing_level > 4 || Targets.size() <= max_os_group_sz * fudgeratio) {
    if (family == AF_INET6)
      os_scan_ipv6(Targets);
    else
      os_scan_ipv4(Targets);
    return OP_SUCCESS;
  }

  /* We need to split it up */
  while (startidx < Targets.size()) {
    int diff = Targets.size() - startidx;
    if (diff > max_os_group_sz * fudgeratio) {
      diff = max_os_group_sz;
    }
    tmpTargets.assign(Targets.begin() + startidx, Targets.begin() + startidx + diff);
    if (family == AF_INET6)
      os_scan_ipv6(Targets);
    else
      os_scan_ipv4(Targets);
    startidx += diff;
  }
  return OP_SUCCESS;
}


/* Performs the OS detection for IPv4 hosts. This method should not be called
 * directly. os_scan() should be used instead, as it handles chunking so
 * you don't do too many targets in parallel */
int OSScan::os_scan_ipv4(std::vector<Target *> &Targets) {
  int itry = 0;
  /* Hosts which haven't matched and have been removed from incompleteHosts because
   * they have exceeded the number of retransmissions the host is allowed. */
  std::list<HostOsScanInfo *> unMatchedHosts;

  /* Check we have at least one target*/
  if (Targets.size() == 0) {
    return OP_FAILURE;
  }

  perf.init();

  OsScanInfo OSI(Targets);
  if (OSI.numIncompleteHosts() == 0) {
    /* no one will be scanned */
    return OP_FAILURE;
  }
  OSI.starttime = o.TimeSinceStart();

  HostOsScan HOS(Targets[0]);

  /* Initialize the pcap session handler in HOS */
  begin_sniffer(&HOS, Targets);
  while (OSI.numIncompleteHosts() != 0) {
#ifdef WIN32
    // Reset system idle timer to avoid going to sleep
    SetThreadExecutionState(ES_SYSTEM_REQUIRED);
#endif
    if (itry > 0)
      sleep(1);
    if (itry == 3)
      usleep(1500000); /* Try waiting a little longer just in case it matters */
    if (o.verbose) {
      char targetstr[128];
      bool plural = (OSI.numIncompleteHosts() != 1);
      if (!plural) {
        (*(OSI.incompleteHosts.begin()))->target->NameIP(targetstr, sizeof(targetstr));
      } else Snprintf(targetstr, sizeof(targetstr), str_37d32_8_hosts_26687924, (int) OSI.numIncompleteHosts());
      log_write(LOG_STDOUT, str_37s32_38_7s92n_350395625, (itry == 0)? str_Initi_10_ating_1430173386 : str_Retry_8_rying_244909990, itry + 1, targetstr);
      log_flush_all();
    }
    startRound(&OSI, &HOS, itry);
    doSeqTests(&OSI, &HOS);
    doTUITests(&OSI, &HOS);
    endRound(&OSI, &HOS, itry);
    expireUnmatchedHosts(&OSI, &unMatchedHosts);
    itry++;
  }

  /* Now move the unMatchedHosts array back to IncompleteHosts */
  if (!unMatchedHosts.empty())
    OSI.incompleteHosts.splice(OSI.incompleteHosts.begin(), unMatchedHosts);

  if (OSI.numIncompleteHosts()) {
    /* For hosts that don't have a perfect match, find the closest fingerprint
     * in the DB and, if we are in debugging mode, print them. */
    findBestFPs(&OSI);
    if (o.debugging > 1)
      printFP(&OSI);
  }

  return OP_SUCCESS;
}


/* Performs the OS detection for IPv6 hosts. This method should not be called
 * directly. os_scan() should be used instead, as it handles chunking so
 * you don't do too many targets in parallel */
int OSScan::os_scan_ipv6(std::vector<Target *> &Targets) {

  /* Object instantiation */
  FPEngine6 fp6;

  /* Safe checks. */
  if (Targets.size() == 0) {
    return OP_FAILURE;
  }

  return fp6.os_scan(Targets);
}


/* This function performs the OS detection. It processes the supplied list of
 * targets and classifies it into two groups: IPv4 and IPv6 targets. Then,
 * OS detection is carried out for those two separate groups. It returns
 * OP_SUCCESS on success or OP_FAILURE in case of error. */
int OSScan::os_scan(std::vector<Target *> &Targets) {
  std::vector<Target *> ip4_targets;
  std::vector<Target *> ip6_targets;
  int res4 = OP_SUCCESS, res6 = OP_SUCCESS;

  /* Make sure we have at least one target */
  if (Targets.size() <= 0)
    return OP_FAILURE;

  /* Classify targets into two groups: IPv4 and IPv6 */
  for (size_t i = 0; i < Targets.size(); i++) {
      if (Targets[i]->af() == AF_INET6)
          ip6_targets.push_back(Targets[i]);
      else
          ip4_targets.push_back(Targets[i]);
  }

  /* Do IPv4 OS Detection */
  if (ip4_targets.size() > 0)
      res4 = this->os_scan_ipv4(ip4_targets);

  /* Do IPv6 OS Detection */
  if (ip6_targets.size() > 0)
      res6 = this->os_scan_ipv6(ip6_targets);

  /* If both scans were successful, return OK */
  if (res4 == OP_SUCCESS && res6 == OP_SUCCESS)
    return OP_SUCCESS;
  else
    return OP_FAILURE;
}
