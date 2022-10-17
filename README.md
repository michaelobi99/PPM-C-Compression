The prediction by partial matching (PPM) is the state of the art for lossless data compression. It uses a finite context length to estimate
the probability of a symbol. 
This implementation (PPMC) uses a trie data structure to model the different contexts and generate probabilities. The proababilities
generated are sent to an arithmetic encoder to encode them into bits.

Compression Efficiency:
I tested the algorithm on several files with very good compression ratios, usually (3-4) : 1, and in one case i got a compression ratio of about
100 (the file content was very skewed).

Memory Usage:
The memory requirement for the PPM is very large, and increases with increasing context length.

Compression speed:
The compression speed of this implementation averages at about 1.3 mb/second.



