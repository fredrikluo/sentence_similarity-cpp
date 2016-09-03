/**
 *  If you want to refactor code in the file, please don't.
 *  All functions in this file are specifically engineered to get least hidden compiler generated code
 *  for most of them are performance crtical.
 *  
 *  That's the reason why they are all static local functions ( it makes sure compiler take them as leaf
 *  functions when optimizing, hence most of them get inlined).
 *
 *  Don't change the structure unless you check with assembly output afterwords;
 */


#include "config.h"
#include "article_similarity.h"
#include <iostream>
#include <math.h>

using namespace std;

static const float ALPHA = 0.2;
static const float BETA = 0.45;
static const float ETA = 0.4;
static const float DELTA = 0.85;
static const float PHI = 0.2;

typedef google::dense_hash_map<int64_t, float> word_sim_cache_t;
word_sim_cache_t g_word_sim_cache;

static float calc_internal(article_entry& entry, article_entry& ref_entry, article_similarity::article_buf_t& buf);

static int r = 1, c = 0;

#ifdef OPA_LOG_OUTPUT

# define OPA_LOG(x) (cout << x)

# define DV(T, v) {\
    for (T::iterator itr = v.begin();\
            itr != v.end();\
            itr ++) {\
        cout << (*itr)->get_word() << ",";\
    }\
    cout << endl;\
}

# define DVF(T, v) {\
    for (T::iterator itr = v.begin();\
            itr != v.end();\
            itr ++) {\
        cout << (*itr) << ",";\
    }\
    cout << endl;\
}
#else
# define OPA_LOG(x)
# define DV(T, v)
# define DVF(T, v)
#endif // OPA_LOG_OUTPUT

#ifdef OPA_LOG_RESULT
# define OPA_LOG_R(x) (cout << x)
#else
# define OPA_LOG_R(x)
#endif // OPA_LOG_RESULT

article_similarity::article_similarity(article_list& list):m_list(list) {
    g_word_sim_cache.set_empty_key(0);
}

void article_similarity::calc(int start, int count) {
    boost::intrusive::list<article_entry>& alist = m_list.get_article_list();
    typedef boost::intrusive::list<article_entry>::iterator lt;

    // Skip all the item before start.
    int s = 0;
    lt cur_itr(alist.begin()), end_itr(alist.end());
    for (; cur_itr != end_itr && start < s ; cur_itr ++, s++);

    // This function is performance critical, it's a O^2 time complexity.
    // We need to squeeze out as much performance as possible.
    if (cur_itr != end_itr) {
        int n = 0;
        for (; cur_itr != end_itr && n < count ; cur_itr ++, n ++) {

#ifdef OPA_LOG_PROGRESS
            if (!(n % 50)) {
                cout << "cache hit:" << ((float)c * 100)/r << ":" << r << ":" << c << ":" << r-c<< ":" << g_word_sim_cache.size() << endl;
                r = 1;
                c = 0;
                cout << "calc ..." << n << endl;
            }
#endif // OPA_LOG_PROGRESS

            for (lt itr = alist.begin(); itr != cur_itr; itr++) {
                calc_internal(*cur_itr, *itr, article_buf);
            }
        }
    }
}

static float word_similarity(op_word* word_1, op_word* word_2) {
    std::vector<op_word::op_syn_node* >& synsets1 = word_1->get_synsets();
    std::vector<op_word::op_syn_node* >& synsets2 = word_2->get_synsets();

    if (synsets1.size() == 0 ||
        synsets2.size() == 0) {
        return 0;
    }

    int64_t key = ((int64_t)word_1->id()) * word_2->id();
    float res = 0;
    r ++;

#ifdef OPA_WITHOUT_HASH_OPT
    word_sim_cache_t::iterator itr = g_word_sim_cache.find(key);
    if (itr != g_word_sim_cache.end()) {
        c ++;
        return itr->second;
    }
#else
    if (g_word_sim_cache.find(key, res)) {
        c++;
        return res;
    }
#endif // OPA_WITHOUT_HASH_OPT

#ifdef OPA_LOG_OUTPUT
    OPA_LOG("cal:"<< word_1->get_word() << " -- " << word_2->get_word() << endl);
    word_1->dump();
    word_2->dump();
#endif // OPA_LOG_OUTPUT

    size_t res_ld = UINT32_MAX, res_hd = UINT32_MAX;

    for (int i = 0 ; i < synsets1.size(); i ++) {
        op_word::op_syn_node* n1 = synsets1[i];
        vector<size_t>& v1 = n1->hypern;
        size_t v1size = v1.size();
        size_t sid_1 = n1->sid;
        for (int j = 0 ; j < synsets2.size(); j ++) {
            // if two synets are the same
            size_t sid_2 = synsets2[j]->sid;
            if (synsets2[j]->sid == sid_1) {
                res_ld = 0;
                res_hd = v1size + 1;
                goto __word_similiarity_break;
            } else if (n1->syn_with_shared_words.find(sid_2) != n1->syn_with_shared_words.end()) {
                OPA_LOG("connected syn:" << sid_1 << "--"<<sid_2 << endl);
                res_ld = 1;
                res_hd = v1size + 1;
            } else {
                vector<size_t>& v2 = synsets2[j]->hypern;
                size_t ld = 0, hd = v1size + 1;
                size_t v2size = v2.size();
                for (int v1i = v1.size() - 1, v2i = v2size -1  ; v1i >= 0 && v2i >=0 ; v1i --, v2i--) {
                    if (v1[v1i] != v2[v2i]) {
                        ld = v1i + v2i + 2;
                        hd = v1size - v1i + 1;
                        break;
                    }
                }

                OPA_LOG(sid_1<<"--"<<sid_2 << "--> ld:" << ld << "hd:" << hd << endl);
                if (ld < res_ld) {
                    OPA_LOG("register:" << i << sid_1<<"--"<<sid_2 << "--> ld:" << ld << "hd:" << hd << endl);
                    res_ld = ld;
                    res_hd = hd;
                }
            }
        }
    }

__word_similiarity_break:
    float hd = res_hd * BETA;
    float ld = res_ld;
    res = exp(-ALPHA * ld) * ((exp(BETA * hd) - exp(-BETA * hd)) /
         (exp(BETA * hd) + exp(-BETA * hd)));
    g_word_sim_cache[key] = res;
    return res;
}

#ifdef DEBUG
void article_similarity::test_word_similarity() {
    typedef struct _wt{
        const char * w1, *w2;
        float r;
    } wt_t;

    vector<wt_t> tv;
#define add_as_test(w1, w2, r) { wt_t w = {w1, w2, r}; tv.push_back(w);}
    add_as_test("as", "ram", 0.21)
    add_as_test("asylum", "fruit", 0.21)
    add_as_test("autograph", "shore", 0.29)
    add_as_test("autograph", "signature", 0.55)
    add_as_test("automobile", "car", 0.64)
    add_as_test("bird", "woodland", 0.33)
    add_as_test("boy", "rooster", 0.53)
    add_as_test("boy", "lad", 0.66)
    add_as_test("boy", "sage", 0.51)
    add_as_test("cemetery", "graveyard", 0.73)
    add_as_test("coast", "forest", 0.36)
    add_as_test("coast", "shore", 0.76)
    add_as_test("cock", "rooster", 1.00)
    add_as_test("cord", "smile", 0.33)
    add_as_test("cord", "string", 0.68)
    add_as_test("cushion", "pillow", 0.66)
    add_as_test("forest", "graveyard", 0.55)
    add_as_test("forest", "woodland", 0.70)
    add_as_test("furnace", "stove", 0.72)
    add_as_test("glass", "tumbler", 0.65)
    add_as_test("grin", "smile", 0.49)
    add_as_test("gem", "jewel", 0.83)
    add_as_test("hill", "woodland", 0.59)
    add_as_test("hill", "mound", 0.74)
    add_as_test("implement", "tool", 0.75)
    add_as_test("journey", "voyage", 0.52)
    add_as_test("magician", "oracle", 0.44)
    add_as_test("magician", "wizard", 0.65)
    add_as_test("midday", "noon", 1.0)
    add_as_test("oracle", "sage", 0.43)
    add_as_test("serf", "slave", 0.39)

    for (vector<wt_t>::iterator itr = tv.begin(); itr != tv.end(); itr ++) {
        string w1(itr->w1);
        string w2(itr->w2);
        float r = word_similarity(op_word::get_word(w1), op_word::get_word(w2));
        OPA_LOG_R("similarity:"<< w1 << ":" << w2 << "--" << itr->r << ":" << r << endl);
    }
}
#endif // DEBUG

static vector<float>& vec_add(vector<float> & s1, vector<float> & s2) {
    for (int i = 0; i < s1.size(); i ++) {
        s1[i] = s1[i] + s2[i];
    }
    return s1;
}

static vector<float>& vec_sub(vector<float> & s1, vector<float> & s2) {
    for (int i = 0; i < s1.size(); i ++) {
        s1[i] = s1[i] - s2[i];
    }
    return s1;
}

static void build_joint_words(vector<op_word*> & entry,
                       boost::unordered_set<op_word*> & uni_words,
                       vector<op_word*>& ref,
                       vector<op_word*>& joint_vec) {
    /*--  Build up joint vector */
    joint_vec = entry;

#ifdef OPA_LOG_JOINT_WORDS
    OPA_LOG("entry:" << endl);
    DV(vector<op_word*>, entry);
    OPA_LOG("uni words:" << endl);
    DV(boost::unordered_set<op_word*>, uni_words);
    OPA_LOG("ref:"<< endl);
    DV(vector<op_word*>, ref);
#endif // OPA_LOG_JOINT_WORDS

    for (int i = 0; i < ref.size(); i ++) {
        if (uni_words.find(ref[i]) == uni_words.end()) {
            joint_vec.push_back(ref[i]);
        }
    }
}

static void build_vectors(vector<op_word*>& words,
                   boost::unordered_set<op_word*> & uni_words,
                   vector<op_word*>& joint_words,
                   vector<float>& semantic_vec,
                   vector<float>& word_order_vec) {
    for (int i = 0; i < joint_words.size(); i ++ ) {
        bool word_contained = uni_words.find(joint_words[i]) != uni_words.end();
        float max_sv = 0;
        int max_x = 0;
        for (int  x = 0; x < words.size(); x ++) {
            OPA_LOG("check:" << joint_words[i]->get_word() << "--" << words[x]->get_word() << endl);
            if (word_contained) {
                if (words[x] == joint_words[i]) {
                    semantic_vec.push_back(words[x]->get_info_content());
                    word_order_vec.push_back(x+1);
                    OPA_LOG("** found, set to 1" << endl);
                    break;
                }
            } else {
                // calculate the similiarity
                float sv = word_similarity(words[x], joint_words[i]);
                if (sv > max_sv) {
                    max_sv = sv;
                    max_x = x;
                }
            }
        }

        if (!word_contained) {
            // Threshold it.
            float r_sv, r_hv = 0;
            OPA_LOG("max_sv:" << max_sv << "max_x:" << max_x << endl);
            if (max_sv < PHI) {
                r_sv = 0;
                OPA_LOG("** no similiar word:" << joint_words[i]->get_word() << endl);
            } else {
                r_sv = max_sv * joint_words[i]->get_info_content() * words[max_x]->get_info_content();
                if (max_sv > ETA) {
                    r_hv = max_x + 1;
                }

                OPA_LOG("**  found most similiar word:" << joint_words[i]->get_word() << "--" << words[max_x]->get_word() << endl);
            }

            semantic_vec.push_back(r_sv);
            word_order_vec.push_back(r_hv);
        }
    }
}


static float dot(vector<float> & v1, vector<float> & v2) {
    float dot = 0;
    for (int i = 0; i < v1.size(); i ++) {
        dot += v1[i] * v2[i];
    }
    return dot;
}

static float norm(vector<float> & vec){
    float r = 0;
    for (int i = 0; i < vec.size(); i ++ ) {
        float v = vec[i];
        r += v * v;
    }
    return sqrt(r);
}

static float calc_similarity(vector<float>& svec1, vector<float>& wvec1, vector<float>& svec2, vector<float>& wvec2) {
    /* -- calcuate the final result */
    float semantic_similarity = dot(svec1, svec2) / (norm(svec1) * norm(svec2));
    float word_order_similarity = 1.0 - norm(vec_sub(wvec1, wvec2)) / norm(vec_add(wvec1, wvec2));
    return  DELTA * semantic_similarity + (1.0 - DELTA) * word_order_similarity;
}

static float calc_internal(article_entry& entry, article_entry& ref_entry, article_similarity::article_buf_t& buf) {
    /* -- reset all the vectors */
    buf.reset();

    /* -- build up the joint words */
    build_joint_words(entry.get_words(), entry.get_uni_words(), ref_entry.get_words(), buf.joint_vec);
    build_vectors(entry.get_words(), entry.get_uni_words(), buf.joint_vec, buf.svec1, buf.wvec1);
    build_vectors(ref_entry.get_words(), ref_entry.get_uni_words(), buf.joint_vec, buf.svec2, buf.wvec2);

    float res =  calc_similarity(buf.svec1, buf.wvec1, buf.svec2, buf.wvec2);
    OPA_LOG_R(entry.get_url() << ":" << ref_entry.get_url() << res << endl);
    return res;
}

#ifdef DEBUG
void article_similarity::test_build_joint_words() {
    /*--  Build up joint vector */
    vector<op_word *> joint_vec;
    article_entry test_ae1;
    article_entry test_ae2;

    std::string text1 = "RAM keeps things being worked with";
    std::string text2 = "The CPU uses RAM as a short-term memory store";

    test_ae1.set_article(text1);
    test_ae2.set_article(text2);
    build_joint_words(test_ae1.get_words(),
                      test_ae1.get_uni_words(),
                      test_ae2.get_words(),
                      joint_vec);

    OPA_LOG("build joint word:" << endl);
    OPA_LOG(text1 << endl);
    OPA_LOG(text2 << endl);

    for (int i = 0; i < joint_vec.size(); i ++) {
        cout << joint_vec[i]->get_word() << "  ";
    }

    cout << endl;

    vector<float> svec1, svec2;
    vector<float> wvec1, wvec2;
    build_vectors(test_ae1.get_words(),
                  test_ae1.get_uni_words(),
                  joint_vec,
                  svec1,
                  wvec1);

    build_vectors(test_ae2.get_words(),
                  test_ae2.get_uni_words(),
                  joint_vec,
                  svec2,
                  wvec2);

    DVF(vector<float>, svec1);
    DVF(vector<float>, wvec1);
    DVF(vector<float>, svec2);
    DVF(vector<float>, wvec2);

    OPA_LOG("similiarity:" << calc_similarity(svec1, wvec1, svec2, wvec2) << endl);
}
#endif // DEBUG

