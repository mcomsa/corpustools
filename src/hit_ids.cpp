#include <Rcpp.h>
using namespace Rcpp;
// [[Rcpp::plugins(cpp11)]]

std::set<int> get_sequence(int &iw, NumericVector &seq_i, NumericVector &pos, NumericVector &term_i){
  std::set<int> seq_i_set;
  double v = term_i[iw];

  int lag = iw;
  for (int si = iw+1; si < seq_i.size(); si++) {
    int posdif = pos[si] - pos[lag];
    if (posdif > 1 or posdif < 0) break;      // next position in seq has to be same (0) or next (1)
    if (seq_i[si] != seq_i[lag]+1) break;
    if (term_i[si] != v) continue;           // seq has to have same term_i

    seq_i_set.insert(si);
    lag = si;
  }
  return seq_i_set;
}

// [[Rcpp::export]]
NumericVector proximity_hit_ids(NumericVector con, NumericVector subcon, NumericVector pos, NumericVector term_i, double n_unique, double window, NumericVector seq_i, LogicalVector replace, bool feature_mode, bool directed) {
  // con: context, subcon: subcontext, pos: term position, term_i: position of term in query, n_unique: number of unique terms in query,
  // window: size of proximity window, seq_i: optionally, a vector that identifies sequences with 1 to last (NA means no seq), assign_once: do not re-use once hit_id is assigned, directed: terms have to be in order from left to right
  double n = pos.size();
  bool use_subcon = subcon.size() > 0;  // use the fact that as.Numeric(NULL) in R returns a vector of length 0 (NULL handling in Rcpp is cumbersome)
  bool use_seq = seq_i.size() > 0;
  bool new_assign;
  NumericVector out(n);

  std::map<int,std::set<int>> tracker;       // keeps track of new unique term_is and their position. When n_unique is reached: returns hit_id and resets

  int iw = 0;
  int hit_id = 1;
  for (int i = 0; i < n; i++) {
    for (iw = i; iw < n; iw++) {
      if (con[iw] != con[i]) break;                // break if different (next) context
      if (pos[iw] - pos[i] > window) break ;     // break if position out of window
      if (use_subcon) {                            // check first term_i
        if (subcon[iw] != subcon[i]) break;
      }

      if (!replace[iw] and !feature_mode) {
        if (out[iw] > 0) continue;               // skip already assigned
        if (tracker.count(term_i[iw])) continue; // skip if unique term_i already observed
      }

      if (directed) {
        if (term_i[iw] > tracker.size() + 1) continue;  // if directed, tracker size will always be 1 behind
      }

      tracker[term_i[iw]].insert(iw);

      if (use_seq) {
        if (!NumericVector::is_na(seq_i[iw])) {
          std::set<int> seq_i_set = get_sequence(iw, seq_i, pos, term_i);   // a simple get sequence that doesn't check for context/window (this is already done to get the seq_i)
          tracker[term_i[iw]].insert(seq_i_set.begin(), seq_i_set.end());
        }
      }

      if (!replace[iw] and !feature_mode) {
        if (tracker.size() == n_unique) break;
      }
    }

    if (tracker.size() == n_unique) {       // if a full set was observed
      new_assign = false;
      for (const auto &positions : tracker){
        for (const auto &position : positions.second) {
          if (out[position] == 0) new_assign = true;
          out[position] = hit_id; // assign hit_id for positions stored in tracker
        }
      }
      if (!feature_mode) {
        hit_id ++;                                 // up counter and reset the tracker
        if (replace[i] and new_assign) i--;    // if term is a replaceable term, repeat the loop until no new hits are found
      }
    }
    tracker.clear();
  }
  return out;
}


bool match_shortest(std::string &x, std::set<std::string> &group_set){
  for (const auto &y : group_set) {
    int length = x.size();
    if (y.size() < length) length = y.size();
    if (x.substr(0,length) == y.substr(0,length)) return true;
  }
  return false;
}

// [[Rcpp::export]]
NumericVector AND_hit_ids(NumericVector con, NumericVector subcon, NumericVector pos, NumericVector term_i, double n_unique, std::vector<std::string> group_i, LogicalVector replace, bool feature_mode) {
  double n = pos.size();
  bool use_subcon = subcon.size() > 0;  // use the fact that as.Numeric(NULL) in R returns a vector of length 0 (NULL handling in Rcpp is cumbersome)
  bool use_group = group_i.size() > 0;
  bool new_assign;
  NumericVector out(n);

  std::map<int,std::set<int> > tracker;       // keeps track of new unique term_is and their position. When n_unique is reached: returns hit_id and resets
  std::map<int,std::set<std::string> > group_tracker;  // in AND, if one term of a group is found, every term must be true (because we search nested)

  int iw = 0;
  int hit_id = 1;
  for (int i = 0; i < n; i++) {
    for (iw = i; iw < n; iw++) {
      if (con[iw] != con[i]) break;                // break if different (next) context
      if (use_subcon) {
        if (subcon[iw] != subcon[i]) break;
      }

      if (!replace[iw] and !feature_mode) {
        if (out[iw] > 0) continue;                                      // skip already assigned
        if (tracker.count(term_i[iw])) {                                // skip if unique term_i already observed...
          if (group_i[iw] == "") continue;                              // but only if there's no group_id...
          //if (group_tracker[term_i[iw]].count(group_i[iw])) continue; // or if group_i is already observed
          if (match_shortest(group_i[iw], group_tracker[term_i[iw]])) continue; // alternative: match on higher level (prevent double counting)
        }
      }

      tracker[term_i[iw]].insert(iw);
      if (group_i[iw] != "") group_tracker[term_i[iw]].insert(group_i[iw]);

      if (!replace[iw] and !feature_mode) {
        if (group_tracker.size() == 0 & tracker.size() == n_unique) break;
      }
    }

    if (tracker.size() == n_unique) {       // if a full set was observed
      new_assign = false;
      for (const auto &positions : tracker){
        for (const auto &position : positions.second) {
          if (out[position] == 0) new_assign = true;
          out[position] = hit_id; // assign hit_id for positions stored in tracker
        }
      }
      if (!feature_mode) {
        hit_id ++;                                 // up counter and reset the tracker
        if (replace[i] and new_assign) i--;    // if term is a replaceable term, repeat the loop until no new hits are found
      }
    }
    tracker.clear();
    group_tracker.clear();
  }
  return out;
}


// [[Rcpp::export]]
NumericVector sequence_hit_ids(NumericVector con, NumericVector subcon, NumericVector pos, NumericVector term_i, double length) {
  double n = pos.size();
  bool use_subcon = subcon.size() > 0;  // use the fact that as.Numeric(NULL) in R returns a vector of length 0 (NULL handling in Rcpp is cumbersome)
  NumericVector out(n);

  double seq_i;
  double fill_i;
  double hit_id = 1;
  for (double i = 0; i < n; i++) {
    for (seq_i = 0; seq_i < length; seq_i++) {
      if (out[i+seq_i] > 0) continue;            // skip already assigned
      if (term_i[i+seq_i] != seq_i+1) break;   // seq_i (starting at 0) should match the number of the word in the sequence (starting at 1)

      if ((pos[i+seq_i] - pos[i] > (0 + seq_i)) | (con[i+seq_i] != con[i])) break;      // there cant be a gap (or same context)
      if (use_subcon) {
        if (subcon[i+seq_i] != subcon[i]) break;
      }

      if (seq_i == length-1) {
        for (fill_i = 0; fill_i < length; fill_i++) {
          out[i+fill_i] = hit_id;
        }
        hit_id++;
        break;
      }
    }
  }
  return out;
}

