//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/
//_/_/ AERA
//_/_/ Autocatalytic Endogenous Reflective Architecture
//_/_/ 
//_/_/ Copyright (c) 2018-2021 Jeff Thompson
//_/_/ Copyright (c) 2018-2021 Kristinn R. Thorisson
//_/_/ Copyright (c) 2018 Jacqueline Clare Mallett
//_/_/ Copyright (c) 2018-2021 Icelandic Institute for Intelligent Machines
//_/_/ http://www.iiim.is
//_/_/ 
//_/_/ Copyright (c) 2012 Jan Koutnik
//_/_/ Center for Analysis and Design of Intelligent Agents
//_/_/ Reykjavik University, Menntavegur 1, 102 Reykjavik, Iceland
//_/_/ http://cadia.ru.is
//_/_/ 
//_/_/ Part of this software was developed by Eric Nivel
//_/_/ in the HUMANOBS EU research project, which included
//_/_/ the following parties:
//_/_/
//_/_/ Autonomous Systems Laboratory
//_/_/ Technical University of Madrid, Spain
//_/_/ http://www.aslab.org/
//_/_/
//_/_/ Communicative Machines
//_/_/ Edinburgh, United Kingdom
//_/_/ http://www.cmlabs.com/
//_/_/
//_/_/ Istituto Dalle Molle di Studi sull'Intelligenza Artificiale
//_/_/ University of Lugano and SUPSI, Switzerland
//_/_/ http://www.idsia.ch/
//_/_/
//_/_/ Institute of Cognitive Sciences and Technologies
//_/_/ Consiglio Nazionale delle Ricerche, Italy
//_/_/ http://www.istc.cnr.it/
//_/_/
//_/_/ Dipartimento di Ingegneria Informatica
//_/_/ University of Palermo, Italy
//_/_/ http://diid.unipa.it/roboticslab/
//_/_/
//_/_/
//_/_/ --- HUMANOBS Open-Source BSD License, with CADIA Clause v 1.0 ---
//_/_/
//_/_/ Redistribution and use in source and binary forms, with or without
//_/_/ modification, is permitted provided that the following conditions
//_/_/ are met:
//_/_/ - Redistributions of source code must retain the above copyright
//_/_/   and collaboration notice, this list of conditions and the
//_/_/   following disclaimer.
//_/_/ - Redistributions in binary form must reproduce the above copyright
//_/_/   notice, this list of conditions and the following disclaimer 
//_/_/   in the documentation and/or other materials provided with 
//_/_/   the distribution.
//_/_/
//_/_/ - Neither the name of its copyright holders nor the names of its
//_/_/   contributors may be used to endorse or promote products
//_/_/   derived from this software without specific prior 
//_/_/   written permission.
//_/_/   
//_/_/ - CADIA Clause: The license granted in and to the software 
//_/_/   under this agreement is a limited-use license. 
//_/_/   The software may not be used in furtherance of:
//_/_/    (i)   intentionally causing bodily injury or severe emotional 
//_/_/          distress to any person;
//_/_/    (ii)  invading the personal privacy or violating the human 
//_/_/          rights of any person; or
//_/_/    (iii) committing or preparing for any act of war.
//_/_/
//_/_/ THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
//_/_/ CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
//_/_/ INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
//_/_/ MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
//_/_/ DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
//_/_/ CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//_/_/ SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
//_/_/ BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
//_/_/ SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
//_/_/ INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
//_/_/ WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
//_/_/ NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//_/_/ OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
//_/_/ OF SUCH DAMAGE.
//_/_/ 
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include "winepi.h"

//#define WINEPI_DEBUG

#ifdef WINEPI_DEBUG
extern std::ofstream* outfile;
#define COUT(x) *outfile << x << std::endl
#else
#define COUT(x) std::cout << x << std::endl
#endif

#define DIFF_CLOCK(t1,t2) (((double) t2-t1) / CLOCKS_PER_SEC)


WinEpi::WinEpi(/*std::multimap<timestamp_t,event_t>& seq_, int win_, double min_fr_, double min_conf_, int max_size_*/)
  : win_(5)
  , min_fr_(0.1)
  , min_conf_(0.5)
  , max_size_(-1)
  // , seq(seq_)
{
  // for(std::multimap<timestamp_t,event_t>::iterator it = seq.seq.begin(); it != seq.seq.end(); ++it)
  // event_types.insert(it->second);
}
/*
template<class InputIterator>
void WinEpi::setSeq(InputIterator it, InputIterator end) {
    seq.init(it, end);
    for(; it != end; ++it) {
        event_types.insert(it->second);
    }
}
*/
void WinEpi::setParams(int win_, double min_fr_, double min_conf_, int max_size_) {
  win_ = win_;
  min_fr_ = min_fr_;
  min_conf_ = min_conf_;
  max_size_ = max_size_;
}

void WinEpi::algorithm_1(std::vector<Rule>& out) {

  std::vector<std::vector<Candidate> > F(1);

  algorithm_2(F);

  // TODO: there must be a way to dramatically speed up the generation of rules below
  for (std::vector<std::vector<Candidate> >::iterator it = F.begin() + 2; it != F.end(); ++it) {
# ifdef WINEPI_DEBUG
    COUT("Finding rules for " << it->size() << " frequent candidates");
    clock_t t1 = clock();
# endif

    for (std::vector<Candidate>::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
      Candidate& a = *it2;

# ifdef WINEPI_DEBUG
      COUT("  Finding subcandidates for candidate of size " << a.size());
      clock_t t3 = clock();
# endif

      double fr_a = fr(a);
      std::vector<Candidate> subcandv;
      strict_subcandidates(a, subcandv);
      for (std::vector<Candidate>::iterator it3 = subcandv.begin(); it3 != subcandv.end(); ++it3) {
        if (std::binary_search(subcandv.begin(), it3, *it3))
          continue;

        double conf = fr_a / fr(*it3);
        if (conf >= min_conf_) {
          out.push_back(Rule(*it3, a, conf));
        }
      }

# ifdef WINEPI_DEBUG
      clock_t t4 = clock();
      COUT("  Time taken: " << DIFF_CLOCK(t3, t4) << " seconds.\n");
# endif
    }

# ifdef WINEPI_DEBUG
    clock_t t2 = clock();
    COUT("Time taken: " << DIFF_CLOCK(t1, t2) << " seconds.\n");
# endif
  }
}

void WinEpi::algorithm_2(std::vector<std::vector<Candidate> >& F) {

  int el = 1;
  std::vector<std::vector<Candidate> > C(2);

  C[el].assign(event_types_.begin(), event_types_.end());

  while (!C[el].empty()) {
# ifdef WINEPI_DEBUG
    COUT("calling algorithm_4 with " << C[el].size() << " candidates of size " << el);
    clock_t t1 = clock();
# endif

    F.resize(F.size() + 1);
    algorithm_4(C[el], min_fr_, F[el]);

# ifdef WINEPI_DEBUG
    clock_t t2 = clock();
    COUT("Time taken: " << DIFF_CLOCK(t1, t2) << " seconds.\n");
# endif

    ++el;
    //# ifdef MAX_SIZE
    if (max_size_ >= 0 && el > max_size_)
      break;
    //# endif

# ifdef WINEPI_DEBUG
    COUT("calling algorithm_3 with " << F[el - 1].size() << " frequent candidates of size " << (el - 1));
    t1 = clock();
# endif

    C.resize(C.size() + 1);
    algorithm_3(F[el - 1], el - 1, C[el]);

# ifdef WINEPI_DEBUG
    t2 = clock();
    COUT("Time taken: " << DIFF_CLOCK(t1, t2) << " seconds.\n");
# endif
  }
}

void WinEpi::algorithm_3(std::vector<Candidate>& F, int el, std::vector<Candidate>& C) {

  int k = -1;
  if (el == 1)
    for (size_t i = 0; i < F.size(); ++i)
      F[i].block_start_ = 0;

  for (size_t i = 0; i < F.size(); ++i) {
    int current_block_start = k + 1;
    size_t j = i;
    while (j < F.size() && F[j].block_start_ == F[i].block_start_) {

      Candidate a;
      for (int x = 1; x <= el; ++x)
        a.set(x, F[i].get(x));
      a.set(el + 1, F[j].get(el));

      bool cont = false;
      for (int y = 1; y < el; ++y) {
        Candidate b;
        for (int x = 1; x < y; ++x)
          b.set(x, a.get(x));
        for (int x = y; x <= el; ++x)
          b.set(x, a.get(x + 1));

        if (!std::binary_search(F.begin(), F.end(), b)) {
          cont = true;
          break;
        }
      }

      ++j;
      if (cont)
        continue;
      ++k;
      a.block_start_ = current_block_start;
      C.push_back(a);
    }
  }
}

void WinEpi::algorithm_4(std::vector<Candidate>& C, double min_fr, std::vector<Candidate>& F) {

  std::map<event_t, int, event_compare> count;
  std::map<std::pair<event_t, size_t>, std::vector<Candidate*>, event_pair > contains;

  for (std::vector<Candidate>::iterator it = C.begin(); it != C.end(); ++it) {
    Candidate& a = *it;
    for (std::map<event_t, size_t, event_compare>::iterator it2 = a.type_count_.begin(); it2 != a.type_count_.end(); ++it2) {
      contains[*it2].push_back(&a);
    }
    a.event_count_ = 0;
    a.freq_count_ = 0;
  }

  for (timestamp_t start = seq_.start_ - win_ + 1; start <= seq_.end_; /*++start*/) {

    std::pair<std::multimap<timestamp_t, event_t>::iterator, std::multimap<timestamp_t, event_t>::iterator> range = seq_.seq_.equal_range(start + win_ - 1);
    for (std::multimap<timestamp_t, event_t>::iterator it = range.first; it != range.second; ++it) {
      event_t& A = it->second;
      // if(count.find(A) != count.end()) {
      ++count[A];
      std::map<std::pair<event_t, size_t>, std::vector<Candidate*> >::iterator it2 = contains.find(std::pair<event_t, size_t>(A, count[A]));
      if (it2 != contains.end()) {
        std::vector<Candidate*>& as = it2->second;
        for (size_t i = 0; i < as.size(); ++i) {
          Candidate& a = *as[i];
          a.event_count_ += count[A];
          if (a.event_count_ == a.size())
            a.inwindow_ = start;
        }
      }
      // }
    }

    range = seq_.seq_.equal_range(start - 1);
    for (std::multimap<timestamp_t, event_t>::iterator it = range.first; it != range.second; ++it) {
      event_t& A = it->second;
      // if(count.find(A) != count.end()) {
      std::map<std::pair<event_t, size_t>, std::vector<Candidate*> >::iterator it2 = contains.find(std::pair<event_t, size_t>(A, count[A]));
      if (it2 != contains.end()) {
        std::vector<Candidate*>& as = it2->second;
        for (size_t i = 0; i < as.size(); ++i) {
          Candidate& a = *as[i];
          if (a.event_count_ == a.size())
            a.freq_count_ += start - a.inwindow_;
          a.event_count_ -= count[A];
        }
      }
      --count[A];
      // }
    }


    if (start == seq_.end_)
      break;

    std::multimap<timestamp_t, event_t>::iterator it = seq_.seq_.lower_bound(start + 1);
    if (it == seq_.seq_.end()) {
      start = seq_.end_;
      continue;
    }
    timestamp_t lodiff = it->first - start;

    it = seq_.seq_.lower_bound(start + win_);
    if (it == seq_.seq_.end()) {
      start += lodiff + 1;
    }
    else {
      timestamp_t hidiff = it->first - start - win_;
      start += min(lodiff, hidiff) + 1;
    }
  }

  timestamp_t n = seq_.end_ - seq_.start_ + win_ - 1;
  for (std::vector<Candidate>::iterator it = C.begin(); it != C.end(); ++it) {
    Candidate& a = *it;
    if ((double)a.freq_count_ / n >= min_fr)
      F.push_back(a);
  }
  std::sort(F.begin(), F.end());
}

double WinEpi::fr(Candidate& a) {
  if (a.freq_count_ < 0) {
    std::vector<Candidate> C, F;
    C.push_back(a);
    algorithm_4(C, 0, F);
    a = F[0];
  }

  return (double)a.freq_count_ / (seq_.end_ - seq_.start_ + win_ - 1);
}




