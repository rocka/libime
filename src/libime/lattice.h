/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_LIBIME_LATTICE_H_
#define _FCITX_LIBIME_LATTICE_H_

#include "languagemodel.h"
#include "libime_export.h"
#include <algorithm>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/type_erased.hpp>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/stringutils.h>
#include <memory>

namespace libime {

class Decoder;
class LatticePrivate;
class SegmentGraphNode;

class SentenceResult {
public:
    typedef std::vector<std::pair<const SegmentGraphNode *, boost::string_view>>
        Sentence;
    SentenceResult(Sentence sentence = {}, float score = 0.0f)
        : sentence_(std::move(sentence)), score_(score) {}

    const std::vector<std::pair<const SegmentGraphNode *, boost::string_view>>
    sentence() const {
        return sentence_;
    }

    float score() const { return score_; }

    bool operator<(const SentenceResult &rhs) const {
        return score_ < rhs.score_;
    }

    bool operator>(const SentenceResult &rhs) const {
        return score_ > rhs.score_;
    }

    std::string toString() const {
        return fcitx::stringutils::join(
            sentence_ | boost::adaptors::transformed(
                            [](const auto &item) { return item.second; }),
            "");
    }

private:
    std::vector<std::pair<const SegmentGraphNode *, boost::string_view>>
        sentence_;
    float score_;
};

class LatticeNode {
public:
    LatticeNode(LanguageModel *model, boost::string_view word, WordIndex idx,
                const SegmentGraphNode *from, const SegmentGraphNode *to,
                float cost = 0, State state = {})
        : word_(word.to_string()), idx_(idx), from_(from), to_(to), cost_(cost),
          state_(state.empty() ? model->nullState() : state) {}

    const std::string &word() const { return word_; }
    float cost() const { return cost_; }

    float score() const { return score_; }
    void setScore(float score) { score_ = score; }

    const SegmentGraphNode *from() const { return from_; }
    const SegmentGraphNode *to() const { return to_; }

    LatticeNode *prev() const { return prev_; }
    void setPrev(LatticeNode *prev) { prev_ = prev; }

    std::string fullWord() const {
        size_t length = 0;
        auto pivot = this;
        while (pivot != nullptr) {
            length += pivot->word().size();
            pivot = pivot->prev();
        }

        std::string result;
        result.resize(length);
        pivot = this;
        while (pivot != nullptr) {
            auto &word = pivot->word();
            length -= word.size();
            std::copy(word.begin(), word.end(), result.begin() + length);
            pivot = pivot->prev();
        }

        return result;
    }

    SentenceResult toSentenceResult() const {
        SentenceResult::Sentence result;
        auto pivot = this;
        // to skip bos
        while (pivot->prev() != nullptr) {
            if (pivot->to()) {
                result.emplace_back(pivot->to(), pivot->word());
            }
            pivot = pivot->prev();
        }

        std::reverse(result.begin(), result.end());
        return {std::move(result), score()};
    }

    State &state() { return state_; }
    WordIndex idx() const { return idx_; }

protected:
    std::string word_;
    WordIndex idx_;
    const SegmentGraphNode *from_, *to_;
    float cost_;
    float score_ = 0.0f;
    State state_;
    LatticeNode *prev_ = nullptr;
};

class LIBIME_EXPORT Lattice {
    friend class Decoder;

public:
    Lattice();
    Lattice(Lattice &&other);
    virtual ~Lattice();

    bool isValid() const { return d_ptr != nullptr; }

    Lattice &operator=(Lattice &&other);

    size_t sentenceSize() const;
    const SentenceResult &sentence(size_t idx) const;

    typedef boost::any_range<LatticeNode, boost::bidirectional_traversal_tag,
                             const LatticeNode &>
        NodeRange;

    NodeRange nodes(const SegmentGraphNode *node) const;

protected:
    Lattice(LatticePrivate *d);

private:
    std::unique_ptr<LatticePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Lattice);
};
}

#endif // _FCITX_LIBIME_LATTICE_H_
