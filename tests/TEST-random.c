#define TC_RANDOM_IMPLEMENTATION
#include "../tc_random.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>

#include <assert.h>

void TEST_h_seed_seq(uint32_t* vals, const uint32_t* expected_vals, size_t elen, const uint32_t* seed, size_t slen)
{
    tcrand_gen_seedseq(vals, elen, seed, slen);
    size_t i;
    for(i = 0; i < elen; i++)
        assert(vals[i] == expected_vals[i]);
}

void TEST_seed_seq_0(void)
{
    static const uint32_t expected_vals[] = {
        UINT32_C(3660036460),UINT32_C(451169086),UINT32_C(2855146599),UINT32_C(1437003431),
        UINT32_C(1807885848),UINT32_C(64637339),UINT32_C(1760393187),UINT32_C(986471539),
        UINT32_C(2832242270),UINT32_C(3882062095),UINT32_C(928856577),UINT32_C(3969104166),
        UINT32_C(2011778112),UINT32_C(3131523103),UINT32_C(2711847005),UINT32_C(4051247784),
        UINT32_C(1081925062),UINT32_C(4063044158),UINT32_C(2119291143),UINT32_C(2880031157),
        UINT32_C(2596872360),UINT32_C(942041019),UINT32_C(3131347846),UINT32_C(3143007182),
        UINT32_C(1703434424),UINT32_C(3939415615),UINT32_C(2200113843),UINT32_C(1269704803),
        UINT32_C(4131268983),UINT32_C(2109017541),UINT32_C(1761134952),UINT32_C(4042861947),
    };
    static const uint32_t seed[] = { 0 };
    uint32_t vals[sizeof(expected_vals) / sizeof(*expected_vals)];
    TEST_h_seed_seq(vals, expected_vals, sizeof(expected_vals) / sizeof(*expected_vals), seed, sizeof(seed) / sizeof(*seed));
}
void TEST_seed_seq_1(void)
{
    static const uint32_t expected_vals[] = {
        UINT32_C(3708618128),UINT32_C(3518499093),UINT32_C(4204267645),UINT32_C(2028922888),
        UINT32_C(114885045),UINT32_C(1741162395),UINT32_C(3713093788),UINT32_C(3067380749),
        UINT32_C(3232339506),UINT32_C(1058224542),UINT32_C(2560240494),UINT32_C(883295019),
        UINT32_C(837674131),UINT32_C(1706961607),UINT32_C(3080172143),UINT32_C(2077856198),
        UINT32_C(2149642671),UINT32_C(2919999737),UINT32_C(3541730402),UINT32_C(1172696066),
        UINT32_C(1235783820),UINT32_C(4144671094),UINT32_C(904131491),UINT32_C(2623041144),
        UINT32_C(2115881493),UINT32_C(4264362510),UINT32_C(1569936585),UINT32_C(3793484295),
        UINT32_C(313390410),UINT32_C(4249129672),UINT32_C(2945119569),UINT32_C(1030806411),
    };
    static const uint32_t seed[] = { 1 };
    uint32_t vals[sizeof(expected_vals) / sizeof(*expected_vals)];
    TEST_h_seed_seq(vals, expected_vals, sizeof(expected_vals) / sizeof(*expected_vals), seed, sizeof(seed) / sizeof(*seed));
}
void TEST_seed_seq_1234(void)
{
    static const uint32_t expected_vals[] = {
        UINT32_C(1537439072),UINT32_C(2320087471),UINT32_C(4256481813),UINT32_C(857771040),
        UINT32_C(672520535),UINT32_C(2802365551),UINT32_C(1008361945),UINT32_C(4149590153),
        UINT32_C(2403935069),UINT32_C(2134441498),UINT32_C(3820975931),UINT32_C(1034376031),
        UINT32_C(2264567795),UINT32_C(4056189613),UINT32_C(380790059),UINT32_C(1437597937),
        UINT32_C(2004823747),UINT32_C(900620582),UINT32_C(1891431024),UINT32_C(859451873),
        UINT32_C(2631406147),UINT32_C(1141945869),UINT32_C(3248590723),UINT32_C(1149585284),
        UINT32_C(2089819374),UINT32_C(591299324),UINT32_C(3957595267),UINT32_C(3394642566),
        UINT32_C(2149432921),UINT32_C(427674817),UINT32_C(483207335),UINT32_C(1083596624),
    };
    static const uint32_t seed[] = { 1, 2, 3, 4 };
    uint32_t vals[sizeof(expected_vals) / sizeof(*expected_vals)];
    TEST_h_seed_seq(vals, expected_vals, sizeof(expected_vals) / sizeof(*expected_vals), seed, sizeof(seed) / sizeof(*seed));
}

void TEST_minstd0(void)
{
    static const uint32_t expected_vals[] = {
        UINT32_C(16807),UINT32_C(282475249),UINT32_C(1622650073),UINT32_C(984943658),
        UINT32_C(1144108930),UINT32_C(470211272),UINT32_C(101027544),UINT32_C(1457850878),
        UINT32_C(1458777923),UINT32_C(2007237709),UINT32_C(823564440),UINT32_C(1115438165),
        UINT32_C(1784484492),UINT32_C(74243042),UINT32_C(114807987),UINT32_C(1137522503),
        UINT32_C(1441282327),UINT32_C(16531729),UINT32_C(823378840),UINT32_C(143542612),
        UINT32_C(896544303),UINT32_C(1474833169),UINT32_C(1264817709),UINT32_C(1998097157),
        UINT32_C(1817129560),UINT32_C(1131570933),UINT32_C(197493099),UINT32_C(1404280278),
        UINT32_C(893351816),UINT32_C(1505795335),UINT32_C(1954899097),UINT32_C(1636807826),
    };
    const uint32_t* seed = tcrand_minstd0_default_seed;
    TC_RandGen rgen;

    tcrand_init_minstd0(&rgen);
    tcrand_seed_raw(&rgen, seed);

    size_t i;
    for(i = 0; i < sizeof(expected_vals) / sizeof(*expected_vals); i++)
    {
        uint32_t v;
        tcrand_next_raw(&rgen, &v);
        assert(v == expected_vals[i]);
    }

    tcrand_deinit(&rgen);
}
void TEST_minstd(void)
{
    static const uint32_t expected_vals[] = {
        UINT32_C(48271),UINT32_C(182605794),UINT32_C(1291394886),UINT32_C(1914720637),
        UINT32_C(2078669041),UINT32_C(407355683),UINT32_C(1105902161),UINT32_C(854716505),
        UINT32_C(564586691),UINT32_C(1596680831),UINT32_C(192302371),UINT32_C(1203428207),
        UINT32_C(1250328747),UINT32_C(1738531149),UINT32_C(1271135913),UINT32_C(1098894339),
        UINT32_C(1882556969),UINT32_C(2136927794),UINT32_C(1559527823),UINT32_C(2075782095),
        UINT32_C(638022372),UINT32_C(914937185),UINT32_C(1931656580),UINT32_C(1402304087),
        UINT32_C(1936030137),UINT32_C(2064876628),UINT32_C(353718330),UINT32_C(1842513780),
        UINT32_C(1947433875),UINT32_C(631416347),UINT32_C(2010567813),UINT32_C(890442452),
    };
    const uint32_t* seed = tcrand_minstd0_default_seed;
    TC_RandGen rgen;

    tcrand_init_minstd(&rgen);
    tcrand_seed_raw(&rgen, seed);

    size_t i;
    for(i = 0; i < sizeof(expected_vals) / sizeof(*expected_vals); i++)
    {
        uint32_t v;
        tcrand_next_raw(&rgen, &v);
        assert(v == expected_vals[i]);
    }

    tcrand_deinit(&rgen);
}
void TEST_mt19937(void)
{
    static const uint32_t expected_vals[] = {
        UINT32_C(3499211612),UINT32_C(581869302),UINT32_C(3890346734),UINT32_C(3586334585),
        UINT32_C(545404204),UINT32_C(4161255391),UINT32_C(3922919429),UINT32_C(949333985),
        UINT32_C(2715962298),UINT32_C(1323567403),UINT32_C(418932835),UINT32_C(2350294565),
        UINT32_C(1196140740),UINT32_C(809094426),UINT32_C(2348838239),UINT32_C(4264392720),
        UINT32_C(4112460519),UINT32_C(4279768804),UINT32_C(4144164697),UINT32_C(4156218106),
        UINT32_C(676943009),UINT32_C(3117454609),UINT32_C(4168664243),UINT32_C(4213834039),
        UINT32_C(4111000746),UINT32_C(471852626),UINT32_C(2084672536),UINT32_C(3427838553),
        UINT32_C(3437178460),UINT32_C(1275731771),UINT32_C(609397212),UINT32_C(20544909),
    };
    const uint32_t* seed = tcrand_mt19937_default_seed;
    TC_RandGen rgen;

    tcrand_init_mt19937(&rgen);
    tcrand_seed_raw(&rgen, seed);

    size_t i;
    for(i = 0; i < sizeof(expected_vals) / sizeof(*expected_vals); i++)
    {
        uint32_t v;
        tcrand_next_raw(&rgen, &v);
        assert(v == expected_vals[i]);
    }

    tcrand_deinit(&rgen);
}
void TEST_mt19937_64(void)
{
    static const uint64_t expected_vals[] = {
        UINT64_C(14514284786278117030),UINT64_C(4620546740167642908),UINT64_C(13109570281517897720),UINT64_C(17462938647148434322),
        UINT64_C(355488278567739596),UINT64_C(7469126240319926998),UINT64_C(4635995468481642529),UINT64_C(418970542659199878),
        UINT64_C(9604170989252516556),UINT64_C(6358044926049913402),UINT64_C(5058016125798318033),UINT64_C(10349215569089701407),
        UINT64_C(2583272014892537200),UINT64_C(10032373690199166667),UINT64_C(9627645531742285868),UINT64_C(15810285301089087632),
        UINT64_C(9219209713614924562),UINT64_C(7736011505917826031),UINT64_C(13729552270962724157),UINT64_C(4596340717661012313),
        UINT64_C(4413874586873285858),UINT64_C(5904155143473820934),UINT64_C(16795776195466785825),UINT64_C(3040631852046752166),
        UINT64_C(4529279813148173111),UINT64_C(3658352497551999605),UINT64_C(13205889818278417278),UINT64_C(17853215078830450730),
        UINT64_C(14193508720503142180),UINT64_C(1488787817663097441),UINT64_C(8484116316263611556),UINT64_C(4745643133208116498),
    };
    const uint32_t* seed = tcrand_mt19937_64_default_seed;
    TC_RandGen rgen;

    tcrand_init_mt19937_64(&rgen);
    tcrand_seed_raw(&rgen, seed);

    size_t i;
    for(i = 0; i < sizeof(expected_vals) / sizeof(*expected_vals); i++)
    {
        uint64_t v;
        tcrand_next_raw(&rgen, &v);
        assert(v == expected_vals[i]);
    }

    tcrand_deinit(&rgen);
}

#include <float.h>
int main(void)
{
    TEST_seed_seq_0();
    TEST_seed_seq_1();
    TEST_seed_seq_1234();

    TEST_minstd0();
    TEST_minstd();
    TEST_mt19937();
    TEST_mt19937_64();

    return 0;
}
