# Tools used

| Tool | Parts assisted | Work and changes |
| --- | --- | --- |
| OpenAI Codex | Benchmark harness, scripts, D6 and documentation | Helped draft and revise code and explanations, check compatibility, correct trace grouping and address review feedback. |
| MinGW g++ | C++ benchmark and teammate implementations | Compiled the harness, AVL, Fenwick and Oracle together using consistent benchmark options. |
| Python | Experiment automation and result processing | Ran experiments, checked outputs, calculated summaries and verified exported result files. |
| Matplotlib | Result figures | Generated latency graphs from raw CSVs with mean values and min/max bars. |
| ReportLab and Poppler | D6 PDF | Generated and visually checked the PDF, then reduced it to two pages and added identification and code references. |
| Windows Performance Recorder and tracerpt | Cache profiling | Recorded hardware-event samples and decoded recordings for benchmark-thread filtering. |
| Portable Zig/Clang | Adversarial input generation | Compiled the supplied generator when its C++17 syntax was incompatible with the installed GCC. |
