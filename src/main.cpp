#include "config.h"
#include "wn.h"
#include <string>
#include "news_processor.h"
#include "article_similarity.h"

using namespace std;

extern "C" int wninit();

#ifdef OPA_SANITY_TEST
void sanity_test(std::string& icfile) {
    std::string str("wives");
    op_word::loaddb(icfile);
    op_word * op = op_word::get_word(str);
    op->dump();
}
#endif // OPA_SANITY_TEST

extern "C" int main(int argc, char* argv[]) {
    /* open database */
    if (wninit()) {
        exit (-1);
    }

    std::string textfile(argv[1]);
    std::string wnfile("./wordnet_db-p.txt");

#ifndef OPA_SANITY_TEST
    news_processor np;
    np.load(textfile, wnfile);
#else
    sanity_test(wnfile);
    article_list al;
    article_similarity as(al);
    as.test_word_similarity();
    as.test_build_joint_words();
#endif  // OPA_SANITY_TEST

#ifdef OPA_DUMP_WORDMAP
    op_word::dump_word_map();
#endif // OPA_DUMP_WORDMAP
    return 0;
}

