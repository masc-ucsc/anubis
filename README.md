
# ANUBIS Benchmarks for Incremental Synthesis #

Synthesis, placement, and routing turnaround times are some of the major
bottlenecks in digital design productivity. Engineers usually wait several
hours to get accurate results to design changes that are often quite small.
Incremental synthesis has emerged as an attempt to reduce these long times, but
research in incremental synthesis currently lacks a consistent benchmark to
enable comparison between different flows and is reflective of real design
changes.  In this paper, we propose ANUBIS, a benchmark for incremental synthesis
based on real designs and real design changes. ANUBUS comes with a standard score
that allows for easily comparing different flows. We evaluate ANUBIS using two
incremental flows for FPGAs to give insights on its usage and reporting.

## Usage ##

### Running the sample flows ###

To run the sample flow, you need to first build yosys and run the benchmarks
with yosys, and then run the appropriate flow. The instructions here assume you
have all the dependencies installed and vivado (or quartus) binaries in your
PATH environment variable. The Makefiles are meant to be run in bash.

```
cd yosys
make
cd ../scripts/yosys
make complete
cd ../vivado # or quartus
make complete
```

The ANUBIS table for the flow will be dumped to STDOUT.

### Creating a custom incremental flow ###

Start from an existing sample case. You need to provide a script for baseline
synthesis, full synthesis of each delta, and incremental synthesis for each
delta (see the appropriate makefile lines).
Then you need to provide a script to collect (for full and incremental
synthesis):

1. Runtime for synthesis, placement, and routing
2. Delay after routing
3. Energy (Power times Frequency) after routing
4. Area after routing

For power, it is fine to use static power analysis (ie. estimated switching
activities) and no leakage power as long as the assumptions are the same for
both the incremental and full synthesis flows.

## Benchmarks ##

The benchmarks consists of a set of designs, design changes and a standard
scoring system. Changes are based on real-world changes collected through
history in the repository of the designs and/or commented out code.

## Scoring ##

The ANUBIS score depends on the runtime, normalized to the
[YOSYS](www.clifford.at/yosys/) runtime, and corrected with the
Quality-of-Results (QoR) degradation for delay, energy (power x frequency) and
area.  For each change, the corrected runtime is calculated as

tau(an) = t_{i}(a_n) + (1+\alpha) x (t_{f}(a_n))/(\alpha + e^((\beta Q_(f)(a_n)) / (Q(i)(a_n))))


Where \\(\alpha ## 10^8\\) and \\(\beta ## 26\\) are two normalization constants.
Then the score for each task (synthesis, placement, routing) and for each QoR
metric (delay, energy, area) is calculated as the geometric mean of the taus of
all the changes and the baseline runtime.

Sample scripts to run and calculate the scores are provided with ANUBIS with

## Reporting ##

To report the scores of an incremental flow, a standard table with breakdown
per task and QoR target is used, with a overall ANUBIS Value (larger is
better). A sample table is show below.

| Task   | Delay     | Energy    | Area      | \gmean         |
| ------ | --------- | --------- | --------- | -------------- |
| Synth  | $an_s_d$  | $an_s_e$  | $an_s_a$  | \gmean(s)      |
| Place  | $an_p_d$  | $an_p_e$  | $an_p_a$  | \gmean(p)      |
| Route  | $an_r_d$  | $an_r_e$  | $an_r_a$  | \gmean(r)      |
| \gmean | \gmean(d) | \gmean(e) | \gmean(a) | \gmean(\gmean) |

If you have a new record and want your results to be displayed here, send your
results, with a binary (or source code) and if we can reproduce your results, we
will include your score in this page with a link to your work.

## More Details ##

For more details, check the [ANUBIS paper](https://users.soe.ucsc.edu/~rafaeltp/files/anubis-iwls2017.pdf).

## Citing ##

If you use ANUBIS to evaluate your flow, please use the following citation:

```bibtex
@inproceedings{anubis:iwls2017,
  author = {Possignolo, Rafael T. and Kabylkas, Nursultan and Renau, Jose},
  title = {{Anubis}: A New Benchmark for Incremental Synthesis},
  booktitle = {Logic Synthesis (IWLS), Proceedings of the 2017 International Workshop on},
  year = {2017},
  month = {Jun},
}
```

## License ##

ANUBIS is distributed under BSD License 2.0, individual benchmarks however may hold
a different license. ANUBIS include the following third part benchmarks:

1. YOSYS:
- Distributed under ISC License

2. ABC:
- Distributed under University of California License

3. MOR1KX :
- Distributed under Open Hardware Description License Version 1.0 (Based on the MPL 2.0 RC2)

4. OR1200:
- Distributed under GNU Lesser General Public License version 2.1

5. ALPHA / DLX:
- Distributed under the Bug UnderGround UMich project (http://bug.eecs.umich.edu/).
Authors do not warrant or assume any legal responsibility for the accuracy,
completeness, or usefulness of this software, nor for any damage derived by its
use.

A copy of the copyright notes is found within the directory of each project.

## BSD License 2.0 ##

Copyright (c) 2017 Regents of the University of California
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the University of California nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL University of California or its affiliates BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

