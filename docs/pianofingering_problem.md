# Problem description

The problem of finding an optimal piano fingering is presented as a combinatorial optimisation problem. The decision variables are discussed below, followed by a description of the objective function and imposed constraints. At the end of this section, the complexity of the problem is discussed.

## 1. Decision variables

A piano fingering indicates which finger should be used to play each note. In piano music, a piece consists of two staves, whereby the notes from the upper staff are played by the right hand and those from the lower staff by the left hand. This means that a solution of the piano fingering problem can be split up in two different sub problems, one for every hand or staff.

The convention when writing fingerings is to label every finger with a number from one up to five, where the first finger is the thumb. This is illustrated in Figure 1.

Figure 1: Convention for piano fingering representations (Yonebayashi et al., 2007b).

## 2. Objective function

We define a set of rules that is used to evaluate the difficulty of a candidate fingering. This is done by measuring the extent to which the rules are followed. An objective function based on an adapted version of their rules is minimised. This function penalises difficult combinations of fingers. In order to properly evaluate biomechanical characteristics of finger positions, a distance matrix is first defined.

### 2.1. Distance matrix

How easy is it to play two subsequent notes with a particular set of fingers? This depends mainly on the finger pair used and the physical distance between the notes on the piano. When using two adjacent fingers, the distance between the notes should be small. To quantify this idea, a distance matrix is defined. MinRel and MaxRel stand for minimal and maximal relaxed distances, which means that two notes separated by this physical distance on the keyboard can easily be played with this finger pair, while still maintaining a relaxed hand. MinPrac and MaxPrac define the largest distances that can be played by each finger pair. All of these distances are user-specific and can easily be adapted in the implementation of the described algorithm. The two final distances are MinComf and MaxComf. These show the distances that can be played comfortably, in the sense of not having to stretch to a maximal spread. They depend on the values of MinPrac and MaxPrac as follows: MaxComf = MaxPrac - 2. MinComf = MinPrac + 2 (for finger pairs including the thumb) or MinComf = MinPrac (for finger pairs not including the thumb).

Table 1. Example distance matrix that describes the pianist's right hand.

| Finger pair   |   MinPrac |   MinComf |   MinRel |   MaxRel |   MaxComf |   MaxPrac |
|---------------|-----------|-----------|----------|----------|-----------|-----------|
| 1-2           |        -8 |        -6 |        1 |        5 |         8 |        10 |
| 1-3           |        -7 |        -5 |        3 |        9 |        12 |        14 |
| 1-4           |        -5 |        -3 |        5 |       11 |        13 |        15 |
| 1-5           |        -2 |         0 |        7 |       12 |        14 |        16 |
| 2-3           |         1 |         1 |        1 |        2 |         5 |         7 |
| 2-4           |         1 |         1 |        3 |        4 |         6 |         8 |
| 2-5           |         2 |         2 |        5 |        6 |        10 |        12 |
| 3-4           |         1 |         1 |        1 |        2 |         2 |         4 |
| 3-5           |         1 |         1 |        3 |        4 |         6 |         8 |
| 4-5           |         1 |         1 |        1 |        2 |         4 |         6 |

When evaluating a fingering, ranges of distances are required in order to know which are the largest spreads that can be attained. For this reason minimal and maximal distances are defined to delimit the relaxed, comfortable and practical ranges.

In this paper, distances are expressed in units that measure how many piano keys one note is separated from another. This is analogous to the definition of Parncutt et al. (1997) and is visualised in Figure 2. Nevertheless, distances between some keys are altered in this research in order to solve the issue noticed by Jacobs (2001). He stated that a missing black key between E and F on one hand, and between B and C on the other hand, does not influence the distance between these respective key pairs. Therefore we introduce two imaginary, unused keys (key 6 and 14 in Figure 2) where a black key is missing. The numbering of the keys (or relevant pitches) continues in the next octave, in such a way that the next key (C) would be attributed a value of 15. This representation system has several advantages. First, the distance between two keys can now correctly be calculated without a bias for the 'missing keys'. Moreover, by calculating modulo 2, one can distinguish between black and white keys. Finally, by calculating modulo 14, the pitch class can be decoded.

Figure 2: Distances on a piano keyboard with additional imaginary black keys.


All finger pairs involving the same finger twice (e.g., fingers 1-1) are set to zero in the distance matrix. An example of the other values in such a distance matrix for a medium right hand is given in Table 1 as an adaptation of Parncutt et al. (1997), taking into account the newly defined keyboard distances. For pianists with smaller hands, tables with reduced absolute values of MinPrac, MaxPrac, MinRel and MaxRel are available in the developed software. Alternatively, users could adapt them according to their personal preferences, as was also suggested by Al Kasimi et al. (2005, 2007).

Interpreting the values in Table 1 is straightforward in combination with the keyboard image in Figure 2. A value of -10 for MinPrac_R(1-2) means that when the thumb is put on A4 (11), the index of a large right hand can be stretched no further than C4 (1), because 1 minus 11 equals -10. In order to calculate the distances for the finger pair where the order of the fingers used in first and second position are swapped, Min and Max have to be interchanged and the values have to be multiplied by -1. E.g., to calculate MinPrac_R(2-1) from finger 2 to 1 of the right hand: take MaxPrac_R(1-2) (11) and multiply by -1. This gives MinPrac_R(2-1) = -11. To deduce the information for the left hand, the order of the finger pair needs to be swapped (e.g., 1-2 becomes 2-1). E.g., to calculate MinPrac_L (2-1) = MinPrac_R (1-2) = -10 (Parncutt et al., 1997).

### 2.2. Rules

The previously defined distance matrix is used to calculate how well a fingering adheres to the set of fingering rules. These rules are listed in Table 2, which includes a description, the associated penalty in the objective function and the originating source for each rule. The application column specifies the relevance of each rule in the context of monophonic, polyphonic or both types of music.

Rules 1, 2, and 13 take the distance matrix into account for consecutive notes. These rules can be applied for each pair of subsequent notes both for monophonic and polyphonic music. **IMPORTANT: These rules apply with cascading (cumulative) penalties.** When a distance violates a threshold, the penalty for that violation is added, and then additional nested checks apply if the distance also violates more severe thresholds. For example, if a distance violates MinRel, it incurs a +1 penalty per unit; if it additionally violates MinComf, it incurs an additional +2 penalty per unit; if it also violates MinPrac, it incurs an additional +10 penalty per unit, for a total of +13 per unit for severe violations.

To prevent unnatural finger crossings in chords, rules 1, 2, and 13 are also applied within a chord in rule 14 to avoid awkward positions. An example of such a position would be the right index finger placed on a lower note than the right thumb within one chord. Hence within rule 14, rules 1 and 2 are applied with doubled scores (and the same cascading logic) to account for the importance of this natural hand position within one chord.

It is important to point out that Parncutt et al. (1997) use the distances MinPrac and MaxPrac in rule 13 as hard constraints. These minimal and maximal distances of each finger pair are not to be violated. If no feasible fingering is found, then that particular (monophonic) part cannot be played legato. In this paper, these practical distances are implemented as soft constraints in the objective function, but they are assigned very high penalties per unit of violation (e.g., +10). This allows to apply the set of rules of Parncutt et al. (1997) on polyphonic music more easily. Otherwise, it would be very hard for the algorithm to find a feasible initial solution.

Rules 3 and 4 also consider the distance matrix and account for hand position changes, which have to be reduced to necessary cases only (Parncutt et al., 1997). Rules 5 up to 11 prevent difficult transitions in monophonic music. The original scores of rule 8 and 9 could respectively add up to five or four between one pair of notes, which is relatively high compared to a score of one or two, resulting from the application of the other rules. Therefore, the score obtained by these rules is divided by two compared to the original suggestion made by Parncutt et al. (1997).

Rule 12 prevents the repetitive use of a finger in combination with a hand position change. For instance, a sequence C4-G4-C5 fingered 2-1-2 in the right hand forces the pianist to reuse the second finger very quickly. This becomes problematic in fast pieces. Therefore, the finger combination 2-1-3, which is favoured by rule 12, offers a better solution. Rule 15 tries to prevent hand position changes when notes with the same pitch are played consecutively. When this rule would not be implemented, consecutive notes which have the same pitch could be played by a different finger.

The logic of the rules and their relative scores were obtained from discussions with professional musicians (Eeman, 2014; Swerts, 2014; Chew, 2014; Leijnse, 2014). The scores resulting from the application of each rule are summed for each hand to form the total difficulty score per hand for a candidate fingering.

Table 2. Set of rules composing the objective function [15].

|   Rule | Application   | Description                                                                                                                                                                                                                                                                                                                                                                                                             | Score                           |
|--------|---------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------|
|      1 | All           | For every unit the distance between two consecutive notes is below MinComf or exceeds MaxComf (cascades with Rules 2 and 13)                                                                                                                                                                                                                                                                                            | +2 (additional)                 |
|      2 | All           | For every unit the distance between two consecutive notes is below MinRel or exceeds MaxRel (cascades with Rules 1 and 13)                                                                                                                                                                                                                                                                                              | +1 (base)                       |
|      3 | Monophonic    | If the distance between a first and third note is below MinComf or exceeds MaxComf : add one point. In addition, if the pitch of the second note is the middle one, is played by the thumb and the distance between the first and third note is below MinPrac or exceeds MaxPrac : add another point. Finally, if the first and third note have the same pitch, but are played by a different finger: add another point | +1 +1 +1                        |
|      4 | Monophonic    | For every unit the distance between a first and third note is below MinComf or exceeds MaxComf                                                                                                                                                                                                                                                                                                                          | +1                              |
|      5 | Monophonic    | For every use of the fourth finger                                                                                                                                                                                                                                                                                                                                                                                      | +1                              |
|      6 | Monophonic    | For the use of the third and the fourth finger (in any consecutive order)                                                                                                                                                                                                                                                                                                                                               | +1                              |
|      7 | Monophonic    | For the use of the third finger on a white key and the fourth finger on a black key (in any consecutive order)                                                                                                                                                                                                                                                                                                          | +1                              |
|      8 | Monophonic    | When the thumb plays a black key: add a half point. Add one more point for a different finger used on a white key just before and one extra for one just after the thumb                                                                                                                                                                                                                                                | +0.5 +1 +1 |
|      9 | Monophonic    | When the fifth finger plays a black key: add zero points. Add one more point for a different finger used on a white key just before and one extra for one just after the fifth finger                                                                                                                                                                                                                                   | +1 +1                           |
|     10 | Monophonic    | For a thumb crossing on the same level (white-white or black-black)                                                                                                                                                                                                                                                                                                                                                     | +1                              |
|     11 | Monophonic    | For a thumb on a black key crossed by a different finger on a white key                                                                                                                                                                                                                                                                                                                                                 | +2                              |
|     12 | Monophonic    | For a different first and third note, played by the same finger, and the second pitch being the middle one                                                                                                                                                                                                                                                                                                              | +1                              |
|     13 | All           | For every unit the distance between two following notes is below MinPrac or exceeds MaxPrac (cascades with Rules 1 and 2)                                                                                                                                                                                                                                                                                               | +10 (additional)                |
|     14 | Polyphonic    | Apply rules 1, 2 (both with doubled scores) and 13 within one chord using cascading logic                                                                                                                                                                                                                                                                                                                               | See Rules 1, 2, 13              |
|     15 | All           | For consecutive slices containing exactly the same notes (with identical pitches), played by a different finger, for each different finger                                                                                                                                                                                                                                                                              | +1                              |

**Note on Cascading Penalties (Rules 1, 2, 13):** The penalties for these rules are cumulative when multiple thresholds are violated. The implementation uses nested checks:

- If `distance < MinRel` or `distance > MaxRel`: add Rule 2 penalty (+1 per unit)
  - Additionally, if `distance < MinComf` or `distance > MaxComf`: add Rule 1 penalty (+2 per unit)
    - Additionally, if `distance < MinPrac` or `distance > MaxPrac`: add Rule 13 penalty (+10 per unit)

For example, a distance 3 units below MinPrac would violate all three thresholds and accumulate: 1 + 2 + 10 = 13 penalty per unit, resulting in a total penalty of 39.

Within chords (Rule 14), the same cascading logic applies, but Rules 1 and 2 are doubled: base penalty becomes +2 (instead of +1), and additional penalty becomes +4 (instead of +2), while Rule 13 remains +10.

Cascading Penalty Table (Medium Hand)

| Pair | -10 | -9 | -8 | -7 | -6 | -5 | -4 | -3 | -2 | -1 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **1-2** | **39** | **26** | 13 | 10 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 2 | 3 | 6 | 9 | **22** | **35** | **48** | **61** | **74** | **87** | **100** | **113** |
| **1-3** | **52** | **39** | **26** | 14 | 11 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 2 | 3 | 6 | 9 | **22** | **35** | **48** | **61** | **74** |
| **1-4** | **78** | **65** | **52** | **39** | **26** | 14 | 11 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 2 | 5 | 8 | **21** | **34** | **47** |
| **1-5** | **117** | **104** | **91** | **78** | **65** | **52** | **39** | 13 | 10 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 2 | 5 | 8 | **21** | **34** |
| **2-3** | **156** | **143** | **130** | **117** | **104** | **91** | **78** | **65** | **52** | **39** | **26** | 13 | 10 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 0 | 1 | 2 | 3 | 6 | 9 | **22** | **35** |
| **2-4** | **156** | **143** | **130** | **117** | **104** | **91** | **78** | **65** | **52** | **39** | **26** | 13 | 10 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 0 | 0 | 1 | 2 | 5 | 8 | **21** | **34** |
| **2-5** | **169** | **156** | **143** | **130** | **117** | **104** | **91** | **78** | **65** | **52** | **39** | **26** | 13 | 10 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 0 | 1 | 2 | 3 | 4 | 7 | 10 |
| **3-4** | **156** | **143** | **130** | **117** | **104** | **91** | **78** | **65** | **52** | **39** | **26** | 13 | 10 | 7 | 6 | 5 | 4 | 3 | **16** | **29** | **42** | **55** | **68** | **81** | **94** | **107** | **120** | **133** | **146** |
| **3-5** | **156** | **143** | **130** | **117** | **104** | **91** | **78** | **65** | **52** | **39** | **26** | 13 | 10 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 0 | 0 | 1 | 2 | 5 | 8 | **21** | **34** |
| **4-5** | **156** | **143** | **130** | **117** | **104** | **91** | **78** | **65** | **52** | **39** | **26** | 13 | 10 | 7 | 6 | 5 | 4 | 3 | **16** | **29** | **42** | **55** | **68** | **81** | **94** | **107** | **120** | **133** | **146** |
## 3. Constraints

In addition to the soft constraints defined in the objective function above, a hard constraint is defined, stating that the same finger cannot be used on two different keys played at the same time. A violation of this constraint results in a rejection of the solution.

## References

- Al Kasimi, A., Nichols, E. &amp; Raphael, C. (2005). Automatic fingering system (AFS). In poster presentation at ismir, london.
- Al Kasimi, A., Nichols, E. &amp; Raphael, C. (2007). A simple algorithm for or automatic generation of polyphonic piano fingerings. In 8th international conference on music information retrieval, vienna.
- Bamberger, J. (1976). The musical significance of beethoven's fingerings in the piano sonatas. In Music forum, iv (pp. 237-280). Columbia University Press.
- Chew, E. (2014). Personal communication.
- Clarke, E., Parncutt, R., Raekallio, M. &amp; Sloboda, J. (1997). Talking fingers: an interview study of pianits' views on fingering. Musicae Scientiae , 1 (1), 87-108.
- Eeman, K. (2014). Personal communication.
- Forney Jr, G. D. (1973). The viterbi algorithm. Proceedings of the IEEE , 61 (3), 268-278.
- Gellrich, M. &amp; Parncutt, R. (1998). Piano technique and fingering in the eighteenth and nineteenth centuries: Bringing a forgotten method back to life. British Journal of Music Education , 15 (1), 5-23.
- Good, M. (2001). Musicxml: An internet-friendly format for sheet music. In Xml conference and expo.
- Hart, M., Bosch, R. &amp; Tsai, E. (2000). Finding optimal piano fingerings. The UMAP Journal , 2 (21), 167-177.
- Herremans, D. &amp; S¨ orensen, K. (2012). Composing first species counterpoint with a variable neighbourhood search algorithm. Journal of Mathematics and the Arts , 6 (4), 169-189.
- Herremans, D. &amp; S¨ orensen, K. (2013). Composing fifth species counterpoint music with a variable neighborhood search algorithm. Expert Systems with Applications , 40 (16), 6427-6437.
- IMSLP. (2014). Petrucci music library. Retrieved 5 March 2014, from http://imslp.org/wiki/ Main Page
- Jacobs, J. P. (2001). Refinements to the ergonomic model for keyboard fingering of parncutt, sloboda, clarke, raekallio, and desain. Music Perception , 18 (4), 505-511.
- Koster, A. M., Hoesel, S. P. &amp; Kolen, A. W. (1998). The partial constraint satisfaction problem: Facets and lifting theorems. Operations Research Letters , 23 (3-5), 89 - 97. Retrieved from http://www.sciencedirect.com/science/article/pii/S0167637798000431 doi: http:// dx.doi.org/10.1016/S0167-6377(98)00043-1
- Koster, A. M., Hoesel, S. P. &amp; Kolen, A. W. (1999). Optimal solutions for frequency assignment problems via tree decomposition. In P. Widmayer, G. Neyer &amp; S. Eidenbenz (Eds.), Graphtheoretic concepts in computer science (Vol. 1665, p. 338-350). Springer Berlin Heidelberg. Retrieved from http://dx.doi.org/10.1007/3-540-46784-X 32 doi: 10.1007/3-540-46784-X 32
- Leijnse, J. (2014). Personal communication.
- Lin, C.-C. &amp; Liu, D. S.-M. (2006). An intelligent virtual piano tutor. In Proceedings of the 2006 acm international conference on virtual reality continuum and its applications (pp. 353-356).
- Louren¸ co, H. R., Martin, O. C. &amp; St¨ utzle, T. (2003). Iterated local search. In Handbook of metaheuristics (pp. 320-353).
- Mladenovi´ c, N. &amp; Hansen, P. (1997). Variable neighborhood search. Computers &amp; Operations Research , 24 (11), 1097-1100.
- Nakamura, E., Ono, N. &amp; Sagayama, S. (2014). Merged-output hmm for piano fingering of both hands. In Proceedings of the 15th international society for music information retrieval conference (pp. 531-536).
- Newman, W. S. (1982). Beethoven's fingerings as interpretive clues. Journal of Musicology , 1 (2), 171-197.
- Palhazi Cuervo, D., Goos, P., S¨ orensen, K. &amp; Arr´ aiz, E. (2014). An iterated local search algorithm for the vehicle routing problem with backhauls. European Journal of Operational Research , 237 (2), 454-464.
- Parncutt, R. (1997). Modeling piano performance: Physics and cognition of a virtual pianist. In Proceedings of int. computer music conference (thessalonki/gk, 1997) (pp. 15-18).
- Parncutt, R., Sloboda, J. A. &amp; Clarke, E. F. (1999). Interdependence of right and left hands in sight-read, written, and rehearsed fingerings of parallel melodic piano music. Australian Journal of Psychology , 51 (3), 204-210.
- Parncutt, R., Sloboda, J. A., Clarke, E. F., Raekallio, M. &amp; Desain, P. (1997). An ergonomic model of keyboard fingering for melodic fragments. Music Perception , 14 (4), 341-382.
- Radicioni, D. P., Anselma, L. &amp; Lombardo, V. (2004). An algorithm to compute fingering for string instruments. In Proceedings of the National Congress of the Associazione Italiana di Scienze Cognitive, Ivrea, Italy.
- Robine, M. (2009). Analyse automatique du doigt´ e au piano. In Proceedings of the journ´ ees d'informatique musicale (pp. 106-112).
- S´ ebastien, V., Ralambondrainy, H., S´ ebastien, O. &amp; Conruyt, N. (2012). Score analyzer: Automatically determining scores difficulty level for instrumental e-learning. In Proceedings of the 13th international society for music information retrieval conference (pp. 571-576).
- Sloboda, J. A., Clarke, E. F., Parncutt, R. &amp; Raekallio, M. (1998). Determinants of finger choice in piano sight-reading. Journal of experimental psychology: Human perception and performance , 24 (1), 185-203.
- S¨ orensen, K. &amp; Glover, F. (2012). Metaheuristics. In S. I. Glass &amp; M. C. Fu (Eds.), Encyclopedia of operations research and management science. New York: Springer.
- Swerts, P. (2014). Personal communication.
- Yonebayashi, Y., Kameoka, H. &amp; Sagayama, S. (2007a). Automatic decision of piano fingering based on a hidden markov models. In IJCAI (pp. 2915-2921).
- Yonebayashi, Y., Kameoka, H. &amp; Sagayama, S. (2007b). Overview of our IJCAI-07 presentation on 'automatic decision of piano fingering based on hidden markov models'. Retrieved 18 April 2014, from http://hil.t.u-tokyo.ac.jp/research/introduction/PianoFingering/ Yonebayashi2007IJCAI-article/index.html