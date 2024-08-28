#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Peregrine.hh"

#include "Domain.hh"

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "USAGE: " << argv[0] << " <data graph> <max size> <support threshold> [vertex-induced] [# threads]" << std::endl;
    return -1;
  }

  const std::string data_graph_name(argv[1]);
  uint32_t k = std::stoi(argv[2]);

  uint64_t threshold = std::stoi(argv[3]);

  size_t nthreads = std::thread::hardware_concurrency();
  bool extension_strategy = Peregrine::PatternGenerator::EDGE_BASED;

  uint32_t step = 1;

  // decide whether user provided # threads or extension strategy
  if (argc == 5)
  {
    std::string arg(argv[4]);
    if (arg.starts_with("v")) // asking for vertex-induced
    {
      extension_strategy = Peregrine::PatternGenerator::VERTEX_BASED;
      step = 2;
    }
    else if (!arg.starts_with("e")) // not asking for edge-induced
    {
      nthreads = std::stoi(arg);
    }
  }
  else if (argc == 6)
  {
    for (std::string arg : {argv[4], argv[5]})
    {
      if (arg.starts_with("v")) // asking for vertex-induced
      {
        extension_strategy = Peregrine::PatternGenerator::VERTEX_BASED;
        step = 2;
      }
      else if (!arg.starts_with("e")) // not asking for edge-induced
      {
        nthreads = std::stoi(arg);
      }
    }
  }

  /* --------------- Peregrine code starts here --------------- */

  const auto view = [](auto &&v) { return v.get_support(); };

  std::vector<uint64_t> supports;
  std::vector<Peregrine::SmallGraph> freq_patterns;

  std::cout << k << "-FSM with threshold " << threshold << std::endl;

  auto ts = utils::get_timestamp();

  Peregrine::DataGraph dg(data_graph_name);

  /* ----- initial sub-graph discovery ----- */
  {
    const auto process = [](auto &&a, auto &&cm) {
      uint32_t merge = cm.pattern[0] == cm.pattern[1] ? 0 : 1;
      a.map(cm.pattern, std::make_pair(cm.mapping, merge));
    };

    auto t1 = utils::get_timestamp();
    std::vector<Peregrine::SmallGraph> patterns = {Peregrine::PatternGenerator::star(2)};
    patterns.front().set_labelling(Peregrine::Graph::DISCOVER_LABELS);
    auto t2 = utils::get_timestamp();
    auto psupps = Peregrine::match<Peregrine::Pattern, DiscoveryDomain<1>, Peregrine::AT_THE_END, Peregrine::UNSTOPPABLE>(dg, patterns, nthreads, process, view);
    auto t3 = utils::get_timestamp();
    for (const auto &[p, supp] : psupps)
    {
      if (supp >= threshold)
      {
        freq_patterns.push_back(p);
        supports.push_back(supp);
      }
    }

    std::cout << "Initial FSM pattern generation time: " << (t2-t1)/1e6 << "s" << std::endl;
    std::cout << "Initial FSM matching time: " << (t3-t2)/1e6 << "s" << std::endl;
    std::cout << "Initial FSM runtime: " << (t3-t1)/1e6 << "s" << std::endl;
  }

  /* ----- Extended Subgraph Discovery ----- */

  uint16_t loop_counter = 0;

  auto t4 = utils::get_timestamp();
  std::vector<Peregrine::SmallGraph> patterns = Peregrine::PatternGenerator::extend(freq_patterns, extension_strategy);
  auto t5 = utils::get_timestamp();
  std::cout << "Loop: " << loop_counter << " FSM pattern generation time: " << (t5-t4)/1e6 << "s" << std::endl;

  const auto process = [](auto &&a, auto &&cm) {
    a.map(cm.pattern, cm.mapping);
  };

  while (step < k && !patterns.empty())
  {
    freq_patterns.clear();
    supports.clear();

    auto t6 = utils::get_timestamp();
    auto psupps = Peregrine::match<Peregrine::Pattern, Domain, Peregrine::AT_THE_END, Peregrine::UNSTOPPABLE>(dg, patterns, nthreads, process, view);
    auto t7 = utils::get_timestamp();
    std::cout << "Loop: " << loop_counter << " FSM matching time: " << (t7-t6)/1e6 << "s" << std::endl;

    for (const auto &[p, supp] : psupps)
    {
      if (supp >= threshold)
      {
        freq_patterns.push_back(p);
        supports.push_back(supp);
      }
    }

    loop_counter += 1;

    auto t8 = utils::get_timestamp();
    patterns = Peregrine::PatternGenerator::extend(freq_patterns, extension_strategy);
    auto t9 = utils::get_timestamp();
    std::cout << "Loop: " << loop_counter << " FSM pattern generation time: " << (t9-t8)/1e6 << "s" << std::endl;

    step += 1;
  }
  auto te = utils::get_timestamp();

  std::cout << freq_patterns.size() << " frequent patterns: " << std::endl;
  for (uint32_t i = 0; i < freq_patterns.size(); ++i)
  {
    std::cout << freq_patterns[i].to_string() << ": " << supports[i] << std::endl;
  }

  std::cout << "Total FSM runtime: " << (te-ts)/1e6 << "s" << std::endl;
  return 0;
}
