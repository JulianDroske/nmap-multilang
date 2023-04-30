#define str_FIN_3_FIN_69611 "FIN"
#define str_SYN_3_SYN_82600 "SYN"
#define str_RST_3_RST_81459 "RST"
#define str_RESET_5_RESET_77866287 "RESET"
#define str_PSH_3_PSH_79525 "PSH"
#define str_PUSH_4_PUSH_2467610 "PUSH"
#define str_ACK_3_ACK_64617 "ACK"
#define str_URG_3_URG_84298 "URG"
#define str_ECE_3_ECE_68455 "ECE"
#define str_CWR_3_CWR_67166 "CWR"
#define str_ALL_3_ALL_64897 "ALL"
#define str_NONE_4_NONE_2402104 "NONE"
#define str_37s32_14_4192n_1997646523 "%s %s ( %s )\n"
#define str_Usage_61_2592n_1231433074 "Usage: nmap [Scan Type(s)] [Options] {target specification}\n"
#define str_TARGE_23_5892n_1589947295 "TARGET SPECIFICATION:\n"
#define str_3232C_52_4692n_1010345248 "  Can pass hostnames, IP addresses, networks, etc.\n"
#define str_3232E_72_5492n_1966309877 "  Ex: scanme.nmap.org, microsoft.com/24, 192.168.0.1; 10.0.0-255.1-254\n"
#define str_32324_58_ks92n_1478905843 "  -iL <inputfilename>: Input from list of hosts/networks\n"
#define str_32324_42_ts92n_292323120 "  -iR <num hosts>: Choose random targets\n"
#define str_32324_65_ks92n_411345408 "  --exclude <host1[,host2][,host3],...>: Exclude hosts/networks\n"
#define str_32324_56_le92n_2145651237 "  --excludefile <exclude_file>: Exclude list from file\n"
#define str_HOST3_17_5892n_2084494039 "HOST DISCOVERY:\n"
#define str_32324_48_an92n_286956782 "  -sL: List Scan - simply list targets to scan\n"
#define str_32324_38_an92n_2017434118 "  -sn: Ping Scan - disable port scan\n"
#define str_32324_57_ry92n_1943342 "  -Pn: Treat all hosts as online -- skip host discovery\n"
#define str_32324_77_ts92n_1183670211 "  -PS/PA/PU/PY[portlist]: TCP SYN/ACK, UDP or SCTP discovery to given ports\n"
#define str_32324_73_es92n_189480285 "  -PE/PP/PM: ICMP echo, timestamp, and netmask request discovery probes\n"
#define str_32324_40_ng92n_15987073 "  -PO[protocol list]: IP Protocol Ping\n"
#define str_32324_70_9392n_1198820794 "  -n/-R: Never do DNS resolution/Always resolve [default: sometimes]\n"
#define str_32324_65_rs92n_630661443 "  --dns-servers <serv1[,serv2],...>: Specify custom DNS servers\n"
#define str_32324_39_er92n_1580194044 "  --system-dns: Use OS's DNS resolver\n"
#define str_32324_45_st92n_1436703483 "  --traceroute: Trace hop path to each host\n"
#define str_SCAN3_18_5892n_633636755 "SCAN TECHNIQUES:\n"
#define str_32324_62_ns92n_438386765 "  -sS/sT/sA/sW/sM: TCP SYN/Connect()/ACK/Window/Maimon scans\n"
#define str_32324_17_an92n_1241803589 "  -sU: UDP Scan\n"
#define str_32324_44_ns92n_715279005 "  -sN/sF/sX: TCP Null, FIN, and Xmas scans\n"
#define str_32324_49_gs92n_1953726934 "  --scanflags <flags>: Customize TCP scan flags\n"
#define str_32324_44_an92n_880940338 "  -sI <zombie host[:probeport]>: Idle scan\n"
#define str_32324_39_ns92n_479286505 "  -sY/sZ: SCTP INIT/COOKIE-ECHO scans\n"
#define str_32324_25_an92n_1064850466 "  -sO: IP protocol scan\n"
#define str_32324_40_an92n_1715604543 "  -b <FTP relay host>: FTP bounce scan\n"
#define str_PORT3_36_5892n_1760119084 "PORT SPECIFICATION AND SCAN ORDER:\n"
#define str_32324_47_ts92n_867746835 "  -p <port ranges>: Only scan specified ports\n"
#define str_32323_66_8992n_724148582 "    Ex: -p22; -p1-65535; -p U:53,111,137,T:21-25,80,139,8080,S:9\n"
#define str_32324_76_ng92n_1753742110 "  --exclude-ports <port ranges>: Exclude the specified ports from scanning\n"
#define str_32324_58_an92n_2100935111 "  -F: Fast mode - Scan fewer ports than the default scan\n"
#define str_32324_49_ze92n_471341561 "  -r: Scan ports sequentially - don't randomize\n"
#define str_32324_57_ts92n_250358139 "  --top-ports <number>: Scan <number> most common ports\n"
#define str_32324_61_6292n_1786077364 "  --port-ratio <ratio>: Scan ports more common than <ratio>\n"
#define str_SERVI_28_5892n_1128473172 "SERVICE/VERSION DETECTION:\n"
#define str_32324_59_fo92n_1947526189 "  -sV: Probe open ports to determine service/version info\n"
#define str_32324_73_4192n_1841779415 "  --version-intensity <level>: Set from 0 (light) to 9 (try all probes)\n"
#define str_32324_62_4192n_1595872032 "  --version-light: Limit to most likely probes (intensity 2)\n"
#define str_32324_55_4192n_358970249 "  --version-all: Try every single probe (intensity 9)\n"
#define str_32324_72_4192n_1382170674 "  --version-trace: Show detailed version scan activity (for debugging)\n"
#define str_SCRIP_14_5892n_144542059 "SCRIPT SCAN:\n"
#define str_32324_39_lt92n_763688496 "  -sC: equivalent to --script=default\n"
#define str_32324_70_of92n_40574829 "  --script=<Lua scripts>: <Lua scripts> is a comma separated list of\n"
#define str_32323_59_es92n_1282776714 "           directories, script-files or script-categories\n"
#define str_32324_67_ts92n_2017898244 "  --script-args=<n1=v1,[n2=v2,...]>: provide arguments to scripts\n"
#define str_32324_66_le92n_389508044 "  --script-args-file=filename: provide NSE script args in a file\n"
#define str_32324_51_ed92n_1046336231 "  --script-trace: Show all data sent and received\n"
#define str_32324_50_4692n_2132646971 "  --script-updatedb: Update the script database.\n"
#define str_32324_57_4692n_2012937130 "  --script-help=<Lua scripts>: Show help about scripts.\n"
#define str_32323_71_or92n_1855939615 "           <Lua scripts> is a comma-separated list of script-files or\n"
#define str_32323_31_4692n_1458430102 "           script-categories.\n"
#define str_OS32D_15_5892n_1295565232 "OS DETECTION:\n"
#define str_32324_27_on92n_1352535111 "  -O: Enable OS detection\n"
#define str_32324_59_ts92n_1454647854 "  --osscan-limit: Limit OS detection to promising targets\n"
#define str_32324_46_ly92n_282322894 "  --osscan-guess: Guess OS more aggressively\n"
#define str_TIMIN_25_5892n_138772817 "TIMING AND PERFORMANCE:\n"
#define str_3232O_76_4492n_320944270 "  Options which take <time> are in seconds, or append 'ms' (milliseconds),\n"
#define str_32323_73_4692n_1358614597 "  's' (seconds), 'm' (minutes), or 'h' (hours) to the value (e.g. 30m).\n"
#define str_32324_51_4192n_935469462 "  -T<0-5>: Set timing template (higher is faster)\n"
#define str_32324_72_es92n_217280263 "  --min-hostgroup/max-hostgroup <size>: Parallel host scan group sizes\n"
#define str_32324_72_on92n_27162692 "  --min-parallelism/max-parallelism <numprobes>: Probe parallelization\n"
#define str_32324_75_es92n_677715995 "  --min-rtt-timeout/max-rtt-timeout/initial-rtt-timeout <time>: Specifies\n"
#define str_32323_30_4692n_835663946 "      probe round trip time.\n"
#define str_32324_74_4692n_1400155631 "  --max-retries <tries>: Caps number of port scan probe retransmissions.\n"
#define str_32324_60_ng92n_1060458805 "  --host-timeout <time>: Give up on target after this long\n"
#define str_32324_69_es92n_27692751 "  --scan-delay/--max-scan-delay <time>: Adjust delay between probes\n"
#define str_32324_72_nd92n_1194082816 "  --min-rate <number>: Send packets no slower than <number> per second\n"
#define str_32324_72_nd92n_1968819703 "  --max-rate <number>: Send packets no faster than <number> per second\n"
#define str_FIREW_36_5892n_223705715 "FIREWALL/IDS EVASION AND SPOOFING:\n"
#define str_32324_62_4192n_1682932523 "  -f; --mtu <val>: fragment packets (optionally w/given MTU)\n"
#define str_32324_57_ys92n_665714811 "  -D <decoy1,decoy2[,ME],...>: Cloak a scan with decoys\n"
#define str_32324_41_ss92n_1302792639 "  -S <IP_Address>: Spoof source address\n"
#define str_32324_39_ce92n_1460386474 "  -e <iface>: Use specified interface\n"
#define str_32324_53_er92n_44206870 "  -g/--source-port <portnum>: Use given port number\n"
#define str_32324_78_es92n_1424803016 "  --proxies <url1,[url2],...>: Relay connections through HTTP/SOCKS4 proxies\n"
#define str_32324_64_ts92n_490630295 "  --data <hex string>: Append a custom payload to sent packets\n"
#define str_32324_72_ts92n_738994472 "  --data-string <string>: Append a custom ASCII string to sent packets\n"
#define str_32324_59_ts92n_792689767 "  --data-length <num>: Append random data to sent packets\n"
#define str_32324_66_ns92n_557944367 "  --ip-options <options>: Send packets with specified ip options\n"
#define str_32324_42_ld92n_1328878973 "  --ttl <val>: Set IP time-to-live field\n"
#define str_32324_72_ss92n_1238722271 "  --spoof-mac <mac address/prefix/vendor name>: Spoof your MAC address\n"
#define str_32324_61_um92n_1643372752 "  --badsum: Send packets with a bogus TCP/UDP/SCTP checksum\n"
#define str_OUTPU_9_5892n_1318460049 "OUTPUT:\n"
#define str_32324_71_4492n_1826062741 "  -oN/-oX/-oS/-oG <file>: Output scan in normal, XML, s|<rIpt kIddi3,\n"
#define str_32323_64_4692n_912059112 "     and Grepable format, respectively, to the given filename.\n"
#define str_32324_61_ce92n_315748810 "  -oA <basename>: Output in the three major formats at once\n"
#define str_32324_69_4192n_997613655 "  -v: Increase verbosity level (use -vv or more for greater effect)\n"
#define str_32324_69_4192n_200982228 "  -d: Increase debugging level (use -dd or more for greater effect)\n"
#define str_32324_64_te92n_648838589 "  --reason: Display the reason a port is in a particular state\n"
#define str_32324_51_ts92n_1441436775 "  --open: Only show open (or possibly open) ports\n"
#define str_32324_54_ed92n_418702823 "  --packet-trace: Show all packets sent and received\n"
#define str_32324_62_4192n_1643294234 "  --iflist: Print host interfaces and routes (for debugging)\n"
#define str_32324_73_es92n_886832549 "  --append-output: Append to rather than clobber specified output files\n"
#define str_32324_47_an92n_521131501 "  --resume <filename>: Resume an aborted scan\n"
#define str_32324_63_rd92n_350193080 "  --noninteractive: Disable runtime interactions via keyboard\n"
#define str_32324_75_ML92n_325108916 "  --stylesheet <path/URL>: XSL stylesheet to transform XML output to HTML\n"
#define str_32324_70_ML92n_129323164 "  --webxml: Reference stylesheet from Nmap.Org for more portable XML\n"
#define str_32324_71_ut92n_1336014630 "  --no-stylesheet: Prevent associating of XSL stylesheet w/XML output\n"
#define str_MISC5_7_5892n_741718650 "MISC:\n"
#define str_32324_28_ng92n_284081629 "  -6: Enable IPv6 scanning\n"
#define str_32324_79_te92n_867189635 "  -A: Enable OS detection, version detection, script scanning, and traceroute\n"
#define str_32324_63_on92n_528708505 "  --datadir <dirname>: Specify custom Nmap data file location\n"
#define str_32324_70_ts92n_675461663 "  --send-eth/--send-ip: Send using raw ethernet frames or IP packets\n"
#define str_32324_58_ed92n_207991478 "  --privileged: Assume that the user is fully privileged\n"
#define str_32324_63_es92n_880120718 "  --unprivileged: Assume the user lacks raw socket privileges\n"
#define str_32324_28_er92n_1657849096 "  -V: Print version number\n"
#define str_32324_37_4692n_85028355 "  -h: Print this help summary page.\n"
#define str_EXAMP_11_5892n_897280073 "EXAMPLES:\n"
#define str_3232n_30_rg92n_755212334 "  nmap -v -A scanme.nmap.org\n"
#define str_3232n_41_7892n_162874813 "  nmap -v -sn 192.168.0.0/16 10.0.0.0/8\n"
#define str_3232n_31_8092n_762693291 "  nmap -v -iR 10000 -Pn -p 80\n"
#define str_SEE32_81_ES92n_1633690106 "SEE THE MAN PAGE (https://nmap.org/book/man.html) FOR MORE OPTIONS AND EXAMPLES\n"
#define str_WARNI_75_4692n_1236354962 "WARNING: Running Nmap setuid, as you are doing, is a major security risk.\n"
#define str_WARNI_75_4692n_1685421280 "WARNING: Running Nmap setgid, as you are doing, is a major security risk.\n"
#define str_Warni_65_2ICMP_266543879 "Warning:  You are not root -- using TCP pingscan rather than ICMP"
#define str_Warni_14_e3245_811811469 "Warning: The -"
#define str_32opt_35_e3245_739363086 " option is deprecated. Please use -"
#define str_Outpu_90_uch46_1783155185 "Output filename begins with '-'. Try '-%s ./%s' if you really want it to be named as such."
#define str_o_1_o_111 "o"
#define str_NAXGS_5_NAXGS_74057905 "NAXGS"
#define str_You32_75_3237s_1080535085 "You are using a deprecated option in a dangerous way. Did you mean: -o%c %s"
#define str_oA_2_oA_3506 "oA"
#define str_Canno_47_out46_1668451089 "Cannot display multiple output types to stdout."
#define str_versi_7_rsion_351608024 "version"
#define str_verbo_7_rbose_351107458 "verbose"
#define str_datad_7_tadir_1443200163 "datadir"
#define str_servi_9_icedb_194184717 "servicedb"
#define str_versi_9_iondb_1407102122 "versiondb"
#define str_debug_5_debug_95458899 "debug"
#define str_help_4_help_3198785 "help"
#define str_iflis_6_flist_1191385285 "iflist"
#define str_relea_14_emory_1013075913 "release-memory"
#define str_nogcc_5_nogcc_104996262 "nogcc"
#define str_max45_12_tries_195217935 "max-os-tries"
#define str_max45_15_elism_662021399 "max-parallelism"
#define str_min45_15_elism_598152937 "min-parallelism"
#define str_timin_6_iming_873664438 "timing"
#define str_max45_15_meout_416791795 "max-rtt-timeout"
#define str_min45_15_meout_1563130527 "min-rtt-timeout"
#define str_initi_19_meout_1087723821 "initial-rtt-timeout"
#define str_exclu_11_efile_1203394134 "excludefile"
#define str_exclu_7_clude_1321148966 "exclude"
#define str_max45_13_group_843982702 "max-hostgroup"
#define str_min45_13_group_187066652 "min-hostgroup"
#define str_open_4_open_3417674 "open"
#define str_scanf_9_flags_1819038166 "scanflags"
#define str_defea_20_limit_2096725307 "defeat-rst-ratelimit"
#define str_defea_21_limit_1982538839 "defeat-icmp-ratelimit"
#define str_host4_12_meout_544803608 "host-timeout"
#define str_scan4_10_delay_1253572229 "scan-delay"
#define str_max45_14_delay_1280428672 "max-scan-delay"
#define str_max45_11_tries_347400897 "max-retries"
#define str_oN_2_oN_3519 "oN"
#define str_oM_2_oM_3518 "oM"
#define str_oG_2_oG_3512 "oG"
#define str_oS_2_oS_3524 "oS"
#define str_oH_2_oH_3513 "oH"
#define str_oX_2_oX_3529 "oX"
#define str_iL_2_iL_3331 "iL"
#define str_iR_2_iR_3337 "iR"
#define str_sI_2_sI_3638 "sI"
#define str_sourc_11_5port_382082653 "source-port"
#define str_rando_15_hosts_1756344089 "randomize-hosts"
#define str_nsock_12_ngine_1320937675 "nsock-engine"
#define str_proxi_7_oxies_308889716 "proxies"
#define str_proxy_5_proxy_106941038 "proxy"
#define str_disco_20_45rst_556583729 "discovery-ignore-rst"
#define str_ossca_12_limit_717728857 "osscan-limit"
#define str_ossca_12_guess_713461365 "osscan-guess"
#define str_fuzzy_5_fuzzy_97805834 "fuzzy"
#define str_packe_12_trace_1550131748 "packet-trace"
#define str_versi_13_trace_1004661876 "version-trace"
#define str_data_4_data_3076010 "data"
#define str_data4_11_tring_595486588 "data-string"
#define str_data4_11_ength_381108817 "data-length"
#define str_send4_8_45eth_14873360 "send-eth"
#define str_send4_7_d45ip_1246446320 "send-ip"
#define str_style_10_sheet_158213710 "stylesheet"
#define str_no45s_13_sheet_453430320 "no-stylesheet"
#define str_webxm_6_ebxml_791784381 "webxml"
#define str_rH_2_rH_3606 "rH"
#define str_vv_2_vv_3776 "vv"
#define str_ff_2_ff_3264 "ff"
#define str_privi_10_leged_1654399021 "privileged"
#define str_unpri_12_leged_467223340 "unprivileged"
#define str_mtu_3_mtu_108462 "mtu"
#define str_appen_13_utput_1106145316 "append-output"
#define str_nonin_14_ctive_878465525 "noninteractive"
#define str_spoof_9_45mac_1181309381 "spoof-mac"
#define str_thc_3_thc_114799 "thc"
#define str_badsu_6_adsum_1396635610 "badsum"
#define str_ttl_3_ttl_115180 "ttl"
#define str_trace_10_route_1005569060 "traceroute"
#define str_reaso_6_eason_934964668 "reason"
#define str_allpo_8_ports_1813830417 "allports"
#define str_versi_17_nsity_1957642502 "version-intensity"
#define str_versi_13_light_1012312227 "version-light"
#define str_versi_11_45all_144080472 "version-all"
#define str_syste_10_45dns_1576733127 "system-dns"
#define str_resol_11_45all_376318228 "resolve-all"
#define str_uniqu_6_nique_840528943 "unique"
#define str_log45_10_rrors_605614192 "log-errors"
#define str_depre_22_class_468379728 "deprecated-xml-osclass"
#define str_dns45_11_rvers_831534874 "dns-servers"
#define str_port4_10_ratio_1209087625 "port-ratio"
#define str_exclu_13_ports_1607471895 "exclude-ports"
#define str_top45_9_ports_1928700100 "top-ports"
#define str_scrip_6_cript_907685685 "script"
#define str_scrip_12_trace_1370777543 "script-trace"
#define str_scrip_15_atedb_232342637 "script-updatedb"
#define str_scrip_11_5args_1756330857 "script-args"
#define str_scrip_16_5file_1775477722 "script-args-file"
#define str_scrip_11_5help_1756527053 "script-help"
#define str_scrip_14_meout_991233877 "script-timeout"
#define str_ip45o_10_tions_1190800310 "ip-options"
#define str_min45_8_5rate_750236749 "min-rate"
#define str_max45_8_5rate_1522715323 "max-rate"
#define str_adler_7_ler32_1140680715 "adler32"
#define str_stats_11_every_1952903099 "stats-every"
#define str_disab_16_5ping_554208585 "disable-arp-ping"
#define str_route_9_45dst_871151355 "route-dst"
#define str_resum_6_esume_934426579 "resume"
#define str_Unpar_49_3237d_908355399 "Unparseable option (dash, not '-') in argument %d"
#define str_46Ab5_49_v5858_156932431 "46Ab:D:d::e:Ffg:hIi:M:m:nO::o:P::p:qRrS:s::T:Vv::"
#define str_Bogus_41_ified_1870406969 "Bogus --script-timeout argument specified"
#define str_Bogus_77_ive41_1583525540 "Bogus --max-os-tries argument specified, must be between 1 and 50 (inclusive)"
#define str_Bogus_64_325ms_897555561 "Bogus --max-rtt-timeout argument specified, must be at least 5ms"
#define str_Since_140_nds46_1216917488 "Since April 2010, the default unit for --max-rtt-timeout is seconds, so your time of \"%s\" is %g seconds. Use \"%sms\" for %g milliseconds."
#define str_WARNI_110_fer46_1096399766 "WARNING: You specified a round-trip time timeout (%ld ms) that is EXTRAORDINARILY SMALL.  Accuracy may suffer."
#define str_Bogus_42_ified_1463888787 "Bogus --min-rtt-timeout argument specified"
#define str_Since_140_nds46_67883230 "Since April 2010, the default unit for --min-rtt-timeout is seconds, so your time of \"%s\" is %g seconds. Use \"%sms\" for %g milliseconds."
#define str_Bogus_65_itive_1674600286 "Bogus --initial-rtt-timeout argument specified.  Must be positive"
#define str_Since_144_nds46_826793648 "Since April 2010, the default unit for --initial-rtt-timeout is seconds, so your time of \"%s\" is %g seconds. Use \"%sms\" for %g milliseconds."
#define str_Warni_59_oup46_186918666 "Warning: You specified a highly aggressive --min-hostgroup."
#define str_4545s_97_23446_570320244 "--scanflags option must be a number between 0 and 255 (inclusive) or a string like \"URGPSHFIN\"."
#define str_Argum_49_32133_1290183045 "Argument to --min-parallelism must be at least 1!"
#define str_Warni_82_ity46_1170486200 "Warning: Your --min-parallelism option is pretty high!  This can hurt reliability."
#define str_Bogus_39_ified_231869334 "Bogus --host-timeout argument specified"
#define str_no32t_10_meout_1802045825 "no timeout"
#define str_Since_142_23446_948604461 "Since April 2010, the default unit for --host-timeout is seconds, so your time of \"%s\" is %.1f hours. If this is what you want, use \"%ss\"."
#define str_ttl32_57_ive41_1798676107 "ttl option must be a number between 0 and 255 (inclusive)"
#define str_nmap4_13_vices_1287604877 "nmap-services"
#define str_nmap4_19_robes_624465194 "nmap-service-probes"
#define str_versi_41_nd329_1531915218 "version-intensity must be between 0 and 9"
#define str_Bogus_38_ied46_1291053259 "Bogus --scan-delay argument specified."
#define str_no32d_8_delay_1528103517 "no delay"
#define str_Since_137_nds46_1849548006 "Since April 2010, the default unit for --scan-delay is seconds, so your time of \"%s\" is %.1f minutes. Use \"%sms\" for %g milliseconds."
#define str_Bogus_42_ied46_1221529566 "Bogus --max-scan-delay argument specified."
#define str_Since_146_23446_2068250045 "Since April 2010, the default unit for --max-scan-delay is seconds, so your time of \"%s\" is %.1f minutes. If this is what you want, use \"%ss\"."
#define str_max45_28_itive_3698915 "max-retries must be positive"
#define str_Unkno_35_3237s_1205999226 "Unknown or non-available engine: %s"
#define str_Inval_33_ation_1574039738 "Invalid proxy chain specification"
#define str_Can39_59_her46_2051458296 "Can't use the --data option(s) multiple times, or together."
#define str_Inval_28_ified_159839228 "Invalid hex string specified"
#define str_WARNI_70_lly46_1236247613 "WARNING: Payloads bigger than 1400 bytes may not be sent successfully."
#define str_strin_38_3237d_2055126865 "string length must be between 0 and %d"
#define str_data4_36_3237d_659040116 "data-length must be between 0 and %d"
#define str_37s_2_37s_50831 "%s"
#define str_https_39_46xsl_1821852628 "https://svn.nmap.org/nmap/docs/nmap.xsl"
#define str_HTML3_124_age46_965917073 "HTML output is not directly supported, though Nmap includes an XSL for transforming XML output into HTML.  See the man page."
#define str_37s46_7_6nmap_94897409 "%s.nmap"
#define str_37s46_8_gnmap_1346723940 "%s.gnmap"
#define str_37s46_6_46xml_695788250 "%s.xml"
#define str_3333G_64_3392n_856952080 "!!Greets to Van Hauser, Plasmoid, Skyper and the rest of THC!!\n"
#define str_Only3_31_lowed_1751724877 "Only one input filename allowed"
#define str_45_1_45_1665 "-"
#define str_r_1_r_114 "r"
#define str_Faile_40_ading_1782591629 "Failed to open input file %s for reading"
#define str_ERROR_99_ted41_62280366 "ERROR: -iR argument must be the maximum number of random IPs you wish to scan (use 0 for unlimited)"
#define str_ERROR_51_cters_970660949 "ERROR: -sI argument must be less than %d characters"
#define str_Data3_45_of328_41531293 "Data payload MTU must be >0 and multiple of 8"
#define str_4545p_40_32141_1352228478 "--port-ratio should be between [0 and 1)"
#define str_Only3_76_mas46_1115489 "Only 1 --exclude-ports option allowed, separate multiple ranges with commas."
#define str_4545t_45_eater_30805304 "--top-ports should be an integer 1 or greater"
#define str_Ip32o_43_2long_972007489 "Ip options can't be more than 40 bytes long"
#define str_Ip32o_58_tes41_864089392 "Ip options must be multiple of 4 (read length is %i bytes)"
#define str_37f_2_37f_50818 "%f"
#define str_Argum_63_umber_1679698621 "Argument to --min-rate must be a positive floating-point number"
#define str_Argum_63_umber_139007921 "Argument to --max-rate must be a positive floating-point number"
#define str_Bogus_38_ified_1263331713 "Bogus --stats-every argument specified"
#define str_Canno_71_ame62_1692843980 "Cannot use --resume with other options. Usage: nmap --resume <filename>"
#define str_Unkno_36_63536_1340361998 "Unknown long option (%s) given@#!$#$"
#define str_Canno_38_can46_2065784724 "Cannot use both -4 and -6 in one scan."
#define str_I32am_117_ort46_542624985 "I am afraid IPv6 is not available because your host doesn't support it or you chose to compile Nmap w/o IPv6 support."
#define str_Your3_144_2help_617318976 "Your argument to -b is b0rked. Use the normal url style:  user:pass@server:port or just use server and use default anon login\n  Use -h for help"
#define str_Inval_31_23446_919656488 "Invalid argument to -d: \"%s\"."
#define str_WARNI_59_ems46_363459739 "WARNING: a source port of zero may not work on all systems."
#define str_See32_51_ons46_801475582 "See the output of nmap -h for a summary of options."
#define str_WARNI_57_3245I_1376138034 "WARNING: identscan (-I) no longer supported.  Ignoring -I"
#define str_i_1_i_105 "i"
#define str_Argum_34_32133_1835371162 "Argument to -M must be at least 1!"
#define str_Warni_93_ility_1503543245 "Warning: Your max-parallelism (-M) option is extraordinarily high, which can hurt reliability"
#define str_m_1_m_109 "m"
#define str_First_75_ead46_1846170035 "First-generation OS detection (-O1) is no longer supported. Use -O instead."
#define str_Unkno_23_45O46_1309489681 "Unknown argument to -O."
#define str_P_1_P_80 "P"
#define str_PE_2_PE_2549 "PE"
#define str_P37c_3_P37c_2434095 "P%c"
#define str_Pn_2_Pn_2590 "Pn"
#define str_Host3_94_wer46_1885741342 "Host discovery disabled (-Pn). All addresses will be marked 'up' and scan times may be slower."
#define str_The32_68_ble46_1647016325 "The -PR option is deprecated. ARP scan is always done when possible."
#define str_Only3_64_mas46_166712823 "Only one -PS option is allowed. Combine port ranges with commas."
#define str_Bogus_25_3237s_370902108 "Bogus argument to -PS: %s"
#define str_Only3_77_mas46_1576754257 "Only one -PB, -PA, or -PT option is allowed. Combine port ranges with commas."
#define str_Bogus_25_3237s_934912850 "Bogus argument to -PA: %s"
#define str_Only3_64_mas46_584109429 "Only one -PU option is allowed. Combine port ranges with commas."
#define str_Bogus_25_3237s_438444518 "Bogus argument to -PU: %s"
#define str_Only3_64_mas46_1418902641 "Only one -PY option is allowed. Combine port ranges with commas."
#define str_Bogus_25_3237s_2057137770 "Bogus argument to -PY: %s"
#define str_Bogus_25_3237s_807897485 "Bogus argument to -PB: %s"
#define str_Only3_68_mas46_1174389604 "Only one -PO option is allowed. Combine protocol ranges with commas."
#define str_Bogus_25_3237s_1989595360 "Bogus argument to -PO: %s"
#define str_Illeg_74_245PO_360995959 "Illegal Argument to -P, use -Pn, -PE, -PS, -PA, -PP, -PM, -PU, -PY, or -PO"
#define str_Only3_63_mas46_1543132267 "Only 1 -p option allowed, separate multiple ranges with commas."
#define str_You32_87_ys92n_348491929 "You can only use the source option once!  Use -D <decoy1> -D <decoy2> etc. for decoys\n"
#define str_An32o_128_can41_885798158 "An option is required for -s, most common are -sT (tcp scan), -sS (SYN scan), -sF (FIN scan), -sU (UDP scan) and -sn (Ping scan)"
#define str_sP_2_sP_3645 "sP"
#define str_sn_2_sn_3675 "sn"
#define str_No32s_48_b4163_212753780 "No scan type 'B', did you mean bounce scan (-b)?"
#define str_sR_2_sR_3647 "sR"
#define str_sV_2_sV_3651 "sV"
#define str_WARNI_89_can46_2500569 "WARNING: -sR is now an alias for -sV and activates version detection as well as RPC scan."
#define str_Scant_27_ed92n_137820015 "Scantype %c not supported\n"
#define str_Paran_8_anoid_1236108956 "Paranoid"
#define str_Sneak_6_neaky_1814054907 "Sneaky"
#define str_Polit_6_olite_1898802355 "Polite"
#define str_Norma_6_ormal_1955878649 "Normal"
#define str_Aggre_10_ssive_1154369500 "Aggressive"
#define str_Insan_6_nsane_2099929270 "Insane"
#define str_Unkno_167_ane41_21872250 "Unknown timing mode (-T argument).  Use either \"Paranoid\", \"Sneaky\", \"Polite\", \"Normal\", \"Aggressive\", \"Insane\" or a number from 0 (Paranoid) to 5 (Insane)"
#define str_w_1_w_119 "w"
#define str_Could_41_iting_2039589606 "Could not assign %s to stdout for writing"
#define str_Inval_31_23446_1687089658 "Invalid argument to -v: \"%s\"."
#define str_Faile_62_3237s_68216691 "Failed to resolve/decode supposed %s source address \"%s\": %s"
#define str_IPv4_4_IPv4_2255333 "IPv4"
#define str_IPv6_4_IPv6_2255335 "IPv6"
#define str_Warni_79_lay46_1875051644 "Warning: --min-parallelism and --max-parallelism are ignored with --scan-delay."
#define str_nmap4_10_s45db_222827374 "nmap-os-db"
#define str_Binar_33_2n37s_109108632 "Binary ip options to be send:\n%s"
#define str_Parse_35_7s92n_1683790829 "Parsed ip options to be send:\n%s\n"
#define str_37Y45_17_3237Z_476781238 "%Y-%m-%d %H:%M %Z"
#define str_Unabl_30_2time_1558150059 "Unable to properly format time"
#define str_Start_29_7s92n_1035678152 "Starting %s %s ( %s ) at %s\n"
#define str_Happy_52_3392n_19614874 "Happy %d%s Birthday to Nmap, may it live to be %d!\n"
#define str_th_2_th_3700 "th"
#define str_st_2_st_3681 "st"
#define str_nd_2_nd_3510 "nd"
#define str_rd_2_rd_3634 "rd"
#define str_Nmap3_121_4692n_1061282699 "Nmap wishes you a merry Christmas! Specify -sX for Xmas Scan (https://nmap.org/book/man-port-scanning-techniques.html).\n"
#define str_Trace_32_2root_1478324267 "Traceroute has to be run as root"
#define str_Trace_37_2scan_930386329 "Traceroute does not support idle scan"
#define str_You32_88_2scan_1047716475 "You cannot use -F (fast scan) or -p (explicit port selection) when not doing a port scan"
#define str_You32_130_ports_539238350 "You cannot use -F (fast scan) with -p (explicit port selection) but see --top-ports and --port-ratio to fast scan a range of ports"
#define str_91P58_6_04593_1849166448 "[P:0-]"
#define str_045_2_045_47793 "0-"
#define str_WARNI_79_ype46_886700281 "WARNING: Your ports include \"T:\" but you haven't specified any TCP scan type."
#define str_WARNI_79_5sU46_626994739 "WARNING: Your ports include \"U:\" but you haven't specified UDP scan with -sU."
#define str_WARNI_80_ype46_228974547 "WARNING: Your ports include \"S:\" but you haven't specified any SCTP scan type."
#define str_WARNI_87_5sO46_1893017445 "WARNING: Your ports include \"P:\" but you haven't specified IP Protocol scan with -sO."
#define str_port3_26_4192n_174891938 "port spec: (%d %d %d %d)\n"
#define str_WARNI_137_ime46_767458754 "WARNING: raw IP (rather than raw ethernet) packet sending attempted on Windows. This probably won't work.  Consider --send-eth next time."
#define str_0_1_0_48 "0"
#define str_Bogus_72_itted_1329826766 "Bogus --spoof-mac value encountered (%s) -- only up to 6 bytes permitted"
#define str_Could_163_hem46_1442997321 "Could not parse as a prefix nor find as a vendor substring the given --spoof-mac argument: %s.  If you are giving hex digits, there must be an even number of them."
#define str_Spoof_57_4192n_1455248066 "Spoofing MAC address %02X:%02X:%02X:%02X:%02X:%02X (%s)\n"
#define str_No32r_20_endor_1093287703 "No registered vendor"
#define str_WARNI_98_ype46_1789956068 "WARNING: a TCP scan type was requested, but no tcp ports were specified.  Skipping this scan type."
#define str_WARNI_100_ype46_1458570648 "WARNING: a SCTP scan type was requested, but no sctp ports were specified.  Skipping this scan type."
#define str_WARNI_91_ype46_18267129 "WARNING: UDP scan was requested, but no udp ports were specified.  Skipping this scan type."
#define str_WARNI_110_ype46_1318658810 "WARNING: protocol scan was requested, but no protocols were specified to be scanned.  Skipping this scan type."
#define str_WARNI_118_ype46_1834563012 "WARNING: a TCP ping scan was requested, but after excluding requested TCP ports, none remain. Skipping this scan type."
#define str_WARNI_118_ype46_1419321796 "WARNING: a UDP ping scan was requested, but after excluding requested UDP ports, none remain. Skipping this scan type."
#define str_WARNI_120_ype46_592866078 "WARNING: a SCTP ping scan was requested, but after excluding requested SCTP ports, none remain. Skipping this scan type."
#define str_WARNI_126_ype46_386968064 "WARNING: a IP Protocol ping scan was requested, but after excluding requested protocols, none remain. Skipping this scan type."
#define str_Could_258_shy46_642686344 "Could not figure out what device to send the packet out on with the source address you gave me!  If you are trying to sp00f your scan, this is normal, just give the -e eth0 or -e ppp0 or whatever.  Otherwise you can still use -e, but I find it kind of fishy."
#define str_I32ca_81_ist63_1899999666 "I cannot figure out what source address to use for device %s, does it even exist?"
#define str_Faile_42_ading_1621337923 "Failed to open exclude file %s for reading"
#define str_me_2_me_3480 "me"
#define str_Can32_36_4692n_1621022983 "Can only use 'ME' as a decoy once.\n"
#define str_rnd_3_rnd_113064 "rnd"
#define str_rnd58_4_rnd58_108656203 "rnd:"
#define str_Rando_40_2IPv4_1738124006 "Random decoys can only be used with IPv4"
#define str_Bad32_22_s9234_1948041383 "Bad 'rnd' decoy \"%s\""
#define str_You32_79_46h41_1899316201 "You are only allowed %d decoys (if you need more redefine MAX_DECOYS in nmap.h)"
#define str_Faile_39_3237s_2069455707 "Failed to resolve decoy host \"%s\": %s"
#define str_You32_65_4692n_214517332 "You have specified some options that require raw socket access.\n"
#define str_These_37_37s46_1875572911 "These options will not be honored %s."
#define str_for32_20_2scan_1613062226 "for TCP Connect scan"
#define str_witho_32_leges_1920802265 "without the necessary privileges"
#define str_Micro_9_osoft_1909739726 "Microsoft"
#define str_Warni_68_4692n_103707179 "Warning: %s may not work correctly on Windows Subsystem for Linux.\n"
#define str_For32_94_ows46_981834790 "For best performance and accuracy, use the native Windows build from %s/download.html#windows."
#define str_n95lo_22_3237s_279517827 "n_localtime failed: %s"
#define str_Can39_21_37s46_222505586 "Can't resolve %s: %s."
#define str_37s92_4_7s92n_1514362758 "%s\n"
#define str_Can39_20_s4146_1070401041 "Can't route %s (%s)."
#define str_37s32_5_3237s_696898911 "%s %s"
#define str_32src_11_3237s_18052346 " srcaddr %s"
#define str_32dir_7_irect_1217061112 " direct"
#define str_32nex_11_3237s_941111545 " nexthop %s"
#define str_92n_2_92n_56437 "\n"
#define str_Faile_50_3237s_2031805610 "Failed to resolve FTP bounce proxy hostname/IP: %s"
#define str_Resol_46_4692n_184454690 "Resolved FTP bounce attack proxy to %s (%s).\n"
#define str_n95ct_18_3237s_1398064245 "n_ctime failed: %s"
#define str_nmapr_7_aprun_2054765981 "nmaprun"
#define str_xml45_14_sheet_1449927974 "xml-stylesheet"
#define str_href_4_href_3211051 "href"
#define str_type_4_type_3575610 "type"
#define str_text4_8_47xsl_1079081375 "text/xsl"
#define str_3237s_32_37s32_2147316421 " %s %s scan initiated %s as: %s "
#define str_scann_7_anner_1910961662 "scanner"
#define str_nmap_4_nmap_3384878 "nmap"
#define str_args_4_args_3002589 "args"
#define str_start_5_start_109757538 "start"
#define str_37lu_3_37lu_1575661 "%lu"
#define str_start_8_rtstr_1316816527 "startstr"
#define str_xmlou_16_rsion_2036620800 "xmloutputversion"
#define str_level_5_level_102865796 "level"
#define str_37d_2_37d_50816 "%d"
#define str_debug_9_gging_197681426 "debugging"
#define str_3532_2_3532_1571905 "# "
#define str_37s32_30_3237s_60890459 "%s %s scan initiated %s as: %s"
#define str_WARNI_113_nyway_47565118 "WARNING: Your specified max_parallel_sockets of %d, but your system says it might only give us %d.  Trying anyway"
#define str_The32_42_7d92n_1135324973 "The max # of sockets we are using is: %d\n"
#define str_45454_47_4592n_1405601852 "--------------- Timing report ---------------\n"
#define str_3232h_30_7d92n_791611374 "  hostgroups: min %d, max %d\n"
#define str_3232r_41_7d92n_1596432061 "  rtt-timeouts: init %d, min %d, max %d\n"
#define str_3232m_43_7d92n_1716361666 "  max-scan-delay: TCP %d, UDP %d, SCTP %d\n"
#define str_3232p_31_7d92n_96282890 "  parallelism: min %d, max %d\n"
#define str_3232m_38_ld92n_1173019197 "  max-retries: %d, host-timeout: %ld\n"
#define str_3232m_30_7g92n_1584988974 "  min-rate: %g, max-rate: %g\n"
#define str_45454_47_4592n_799970264 "---------------------------------------------\n"
#define str_host_4_host_3208616 "host"
#define str_Canno_86_6292n_1663934835 "Cannot get hostname!  Try using -S <my_IP_address> or -e <interface to scan through>\n"
#define str_WARNI_131_s6246_885208666 "WARNING: We could not determine for sure which interface to use, so we are guessing %s .  If this is wrong, use -S <my_IP_address>."
#define str_Do32n_46_arget_836042017 "Do not have appropriate device name for target"
#define str_start_9_ttime_2128341457 "starttime"
#define str_endti_7_dtime_1606289880 "endtime"
#define str_timed_8_edout_2076882761 "timedout"
#define str_true_4_true_3569038 "true"
#define str_Skipp_38_ut92n_1954912945 "Skipping host %s due to host timeout\n"
#define str_Host5_32_ut92n_85563657 "Host: %s (%s)\tStatus: Timeout\n"
#define str_Could_24_2file_1537731569 "Could not mmap() %s file"
#define str_Outpu_46_uming_354104235 "Output file %s is too short -- no use resuming"
#define str_32as5_5_s5832_1295575405 " as: "
#define str_Unabl_80_ile63_1298874226 "Unable to parse supposed log file %s.  Are you sure this is an Nmap output file?"
#define str_Unabl_53_ote46_21829027 "Unable to parse supposed log file %s: unclosed quote."
#define str_38quo_6_uot59_196178450 "&quot;"
#define str_38_1_38_1637 "&"
#define str_Unabl_44_Sorry_1555387070 "Unable to parse supposed log file %s.  Sorry"
#define str_Unabl_144_g9234_497295213 "Unable to parse supposed log file %s.  Perhaps the Nmap execution had not finished at least one host?  In that case there is no use \"resuming\""
#define str_nmap3_21_put32_178293898 "nmap --append-output "
#define str_0verf_8_rfl0w_1893228160 "0verfl0w"
#define str_45456_3_54562_1539264254 "-->"
#define str_4545r_17_hosts_1432200229 "--randomize-hosts"
#define str_WARNI_161_2once_903358799 "WARNING: You are attempting to resume a scan which used --randomize-hosts.  Some hosts in the last randomized batch may be missed and others may be repeated once"
#define str_454_2_454_51667 "-4"
#define str_456_2_456_51669 "-6"
#define str_92nHo_8_t5832_2071147137 "\nHost: "
#define str_Unabl_54_Sorry_1759374272 "Unable to parse ip (%s) in supposed log file %s. Sorry"
#define str_92n60_18_19234_770632982 "\n<address addr=\""
#define str_92343_20_49234_1909062616 "\" addrtype=\"ipv4\""
#define str_92343_20_69234_1910909658 "\" addrtype=\"ipv6\""
#define str_Unabl_52_Sorry_908703803 "Unable to parse ip (%s) supposed log file %s.  Sorry"
#define str_92nNm_23_for32_476287223 "\nNmap scan report for "
#define str_Unabl_55_Sorry_1347992705 "Unable to parse ip (%s) in supposed log file %s.  Sorry"
#define str_Warni_139_ing46_1132107833 "Warning: You asked for --resume but it doesn't look like any hosts in the log file were successfully scanned.  Starting from the beginning."
#define str_37s58_28_7ld41_2119615752 "%s: error in munmap(%p, %ld)"
#define str_37s92_12_9237s_1256987086 "%s\\nmap\\%s"
#define str_37s47_11_4737s_1378366186 "%s/.nmap/%s"
#define str_37s47_5_4737s_695826435 "%s/%s"
#define str_NMAPD_7_APDIR_1522232449 "NMAPDIR"
#define str_37s47_19_4737s_567854901 "%s/../share/nmap/%s"
#define str_46473_4_4737s_501237738 "./%s"
#define str_Warni_187_o4146_1093149420 "Warning: File %s exists, but Nmap is using %s for security and consistency reasons.  set NMAPDIR=. to give priority to files in your local directory (may affect the other data files too)."
#define str_Fetch_20_7s92n_1704287826 "Fetchfile found %s\n"
#define str_40_1_40_1660 "("
#define str_41_1_41_1661 ")"
#define str_nmap4_12_lua45_760934211 "nmap-liblua-"
#define str_liblu_7_lua45_852538644 "liblua-"
#define str_liblu_6_iblua_1103022253 "liblua"
#define str_opens_8_ssl45_1564780931 "openssl-"
#define str_opens_7_enssl_1263174782 "openssl"
#define str_nmap4_13_sh245_1982079583 "nmap-libssh2-"
#define str_libss_8_sh245_857657840 "libssh2-"
#define str_libss_7_bssh2_166255407 "libssh2"
#define str_nmap4_10_ibz45_316514875 "nmap-libz-"
#define str_libz4_5_ibz45_1103010858 "libz-"
#define str_libz_4_libz_3321493 "libz"
#define str_nmap4_13_cre45_2082396451 "nmap-libpcre-"
#define str_libpc_8_cre45_757340972 "libpcre-"
#define str_01234_10_56789_1584875013 "0123456789"
#define str_40unk_9_own41_1350913355 "(unknown)"
#define str_44_1_44_1664 ","
#define str_nmap4_5_map45_1042097873 "nmap-"
#define str_nmap4_13_net45_1878806896 "nmap-libdnet-"
#define str_libdn_8_net45_423577023 "libdnet-"
#define str_ipv6_4_ipv6_3239399 "ipv6"
#define str_37s32_22_4192n_171502282 "%s version %s ( %s )\n"
#define str_Platf_14_7s92n_1138687771 "Platform: %s\n"
#define str_Compi_14_ith58_1610534727 "Compiled with:"
#define str_3237s_3_3237s_48639952 " %s"
#define str_Compi_17_out58_304893797 "Compiled without:"
#define str_Avail_29_7s92n_1909484368 "Available nsock engines: %s\n"