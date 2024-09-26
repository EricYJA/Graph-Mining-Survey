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


  const auto view = [](auto &&v) { return v.get_support(); };

  std::vector<uint64_t> supports;
  std::vector<Peregrine::SmallGraph> freq_patterns;
  std::vector<Peregrine::SmallGraph> freq_edge_patterns;

  std::cout << k << "-FSM with threshold " << threshold << std::endl;

  Peregrine::DataGraph dg(data_graph_name);

  printf("Data graph size |V| = %u |E| = %lu\n", dg.get_vertex_count(), dg.get_edge_count());

  // initial discovery
  auto ts = utils::get_timestamp();
  {
    const auto process = [](auto &&a, auto &&cm) {
      uint32_t merge = cm.pattern[0] == cm.pattern[1] ? 0 : 1;
      a.map(cm.pattern, std::make_pair(cm.mapping, merge));
    };

    auto t1 = utils::get_timestamp();
    std::vector<Peregrine::SmallGraph> patterns = {Peregrine::PatternGenerator::star(2)};
    auto t2 = utils::get_timestamp();
    std::cout << "init pattern generation time " << (t2-t1)/1e6 << "s" << std::endl;

    patterns.front().set_labelling(Peregrine::Graph::DISCOVER_LABELS);

    printf("Initial unlabeled pattern size: %lu\n", patterns.size());

    auto t3 = utils::get_timestamp();
    auto psupps = Peregrine::match<Peregrine::Pattern, DiscoveryDomain<1>, Peregrine::AT_THE_END, Peregrine::UNSTOPPABLE>(dg, patterns, nthreads, process, view);
    auto t4 = utils::get_timestamp();
    std::cout << "init pattern matching time " << (t4-t3)/1e6 << "s" << std::endl;

    printf("Initial discovery pattern size: %lu\n", psupps.size());

    for (const auto &[p, supp] : psupps)
    {
      if (supp >= threshold)
      {
        freq_patterns.push_back(p);
        freq_edge_patterns.push_back(p);
        supports.push_back(supp);
      }
    }

    printf("Initial frequent pattern size: %lu\n", freq_patterns.size());
  }

  uint16_t extend_counter = 0;
  
  auto t5 = utils::get_timestamp();
  std::vector<Peregrine::SmallGraph> patterns = Peregrine::PatternGenerator::extend(freq_patterns, extension_strategy, true, freq_edge_patterns);
  std::vector<Peregrine::SmallGraph> patterns_unlabeled = Peregrine::PatternGenerator::all(3, extension_strategy, false);

  printf("patterns num_vertices: %lu\n", patterns[0].num_vertices());
  printf("patterns_unlabeled size: %lu\n", patterns_unlabeled.size());

  auto t6 = utils::get_timestamp();
  std::cout << "pattern extension level " << extend_counter << " time " << (t6-t5)/1e6 << "s" << std::endl;

  printf("Initial extended pattern size: %lu\n", patterns.size());

  const auto process = [](auto &&a, auto &&cm) {
    a.map(cm.pattern, cm.mapping);
  };

  while (step < k && !patterns.empty())
  {
    freq_patterns.clear();
    supports.clear();

    auto t7 = utils::get_timestamp();
    auto psupps = Peregrine::match<Peregrine::Pattern, Domain, Peregrine::AT_THE_END, Peregrine::UNSTOPPABLE>(dg, patterns, nthreads, process, view);
    auto t8 = utils::get_timestamp();
    std::cout << "pattern matching level " << step << " time " << (t8-t7)/1e6 << "s" << std::endl;

    printf("Level %u pattern size: %lu\n", step, psupps.size());

    extend_counter += 1;

    for (const auto &[p, supp] : psupps)
    {
      if (supp >= threshold)
      {
        freq_patterns.push_back(p);
        supports.push_back(supp);
      }
    }

    printf("Level %u frequent pattern size: %lu\n", step, freq_patterns.size());

    auto t9 = utils::get_timestamp();
    patterns = Peregrine::PatternGenerator::extend(freq_patterns, extension_strategy, true, freq_edge_patterns);
    auto t10 = utils::get_timestamp();
    std::cout << "pattern extension level " << extend_counter << " time " << (t10-t9)/1e6 << "s" << std::endl;

    printf("patterns num_vertices: %lu\n", patterns[0].num_vertices());

    printf("Level %u extended pattern size: %lu\n", step, patterns.size());

    step += 1;
  }
  auto te = utils::get_timestamp();

  std::cout << freq_patterns.size() << " frequent patterns: " << std::endl;
  for (uint32_t i = 0; i < freq_patterns.size(); ++i)
  {
    std::cout << freq_patterns[i].to_string() << ": " << supports[i] << std::endl;
  }

  std::cout << "finished in " << (te-ts)/1e6 << "s" << std::endl;
  return 0;
}
