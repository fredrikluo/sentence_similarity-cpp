#ifndef ARTICLE_SIMILARITY
# define ARTICLE_SIMILARITY

#include "article_list.h"
#include "sparsehash/dense_hash_map"

class article_similarity {
public:
    article_similarity(article_list& list);
    void calc(int start, int count);

#ifdef DEBUG
    static void test_word_similarity();
    static void test_build_joint_words();
#endif //DEBUG

    struct article_buf_t {
        void reset() {
            joint_vec.clear();
            svec1.clear();
            svec2.clear();
            wvec1.clear();
            wvec2.clear();
        }
        // Buffers
        std::vector<op_word*> joint_vec;
        std::vector<float> svec1, svec2, wvec1, wvec2;
    } article_buf;

private:
    article_list& m_list;
};

#endif // ARTICLE_SIMILARITY
