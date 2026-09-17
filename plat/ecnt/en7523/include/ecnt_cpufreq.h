
#ifndef _ECNT_CPUFREQ_H
#define _ECNT_CPUFREQ_H

enum e_cpu_freq {
    cpu_freq_500M=0,
    cpu_freq_550M,
    cpu_freq_600M,
    cpu_freq_650M,
    cpu_freq_700M,
    cpu_freq_750M,
    cpu_freq_800M,
    cpu_freq_850M,
    cpu_freq_900M,
    cpu_freq_950M,
	cpu_freq_1000M,
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583)
    cpu_freq_1050M,
    cpu_freq_1100M,
    cpu_freq_1150M,
    cpu_freq_1200M,
    /* Overclocking steps (outside the vendor guaranteed range).
     * Reachable from the ATF SiP frequency-scaling call only; the boot
     * frequency is still the package default set by ecnt_cpu_speedup().
     */
    cpu_freq_1250M,
    cpu_freq_1300M,
    cpu_freq_1350M,
    cpu_freq_1400M,
    cpu_freq_1450M,
    cpu_freq_1500M,
    cpu_freq_1550M,
    cpu_freq_1600M,
#elif !defined(TCSUPPORT_CPU_AN7552)
    /*
     * EN7523 family (EN7523 / EN7562 / EN7529...).
     *
     * The ARM PLL is programmed through the SYSPLL PCW register, whose vendor
     * table covers 500..1200MHz in 50MHz steps, so these indices are valid.
     * They must stay in sync with cpu_freq_config_xtal25M/xtal20M[] and with
     * the non-secure world: airoha-cpufreq / clk-en7523 turn a rate into an
     * index with state = (rate - 500MHz) / 50MHz, so asking for e.g. 1.0GHz
     * (EN7562CT) sends index 10 here; rejecting it floods the console with
     * "ERROR: invalid cpuFreq:10 (valid range: 0~9)" on every cpufreq update.
     * AN7552 stops at 1.0GHz, hence the #elif above.
     */
    cpu_freq_1050M,
    cpu_freq_1100M,
    cpu_freq_1150M,
    cpu_freq_1200M,
#endif
    cpu_freq_last
};

enum e_clk_src {
    clk_src_xtal=0,
    clk_src_armpll,
    clk_src_pll1,
    clk_src_pll2,
    clk_src_last
};

enum cpu_domain_clk_gating {
    cpu_clk_armpll,
    cpu_clk_pll1,
    cpu_clk_pll2,
    cpu_clk_armpll_div2
};

enum e_div_sel {
    div_sel_1=0,
    div_sel_2,
    div_sel_4,
    div_sel_6,
    div_sel_last
};

extern int en7523_armpll_set(enum e_cpu_freq cpuFreq);
extern void set_cpu_domain_clk_gating(enum cpu_domain_clk_gating pll, int isEnable);
unsigned int curr_armpll_clk_get (void);
int an7552_bootup_clk_src_switch(enum e_cpu_freq cpuFreq);

enum e_cpu_freq cpu_freq_enum_get(unsigned int armpll_clk);
void ecnt_cpu_freq_info_dump(void);

#endif /* _ECNT_CPUFREQ_H */
