/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/vault/tools/commander.h"

#include "maidsafe/vault/tools/tools_exception.h"

namespace maidsafe {

namespace vault {

namespace tools {

namespace {

PmidVector GetJustPmids(const KeyChainVector& keychains) {
  PmidVector pmids;
  for (auto& keychain : keychains)
    pmids.push_back(keychain.pmid);
  return pmids;
}

}  // namespace

namespace fs = boost::filesystem;
namespace po = boost::program_options;

bool SelectedOperationsContainer::InvalidOptions(
    const po::variables_map& variables_map,
    const std::vector<boost::asio::ip::udp::endpoint> &peer_endpoints) {
  do_create = variables_map.count("create") != 0;
  do_load = variables_map.count("load") != 0;
  do_delete = variables_map.count("delete") != 0;
  do_bootstrap = variables_map.count("bootstrap") != 0;
  do_store = variables_map.count("store") != 0;
  do_verify = variables_map.count("verify") != 0;
  do_test = variables_map.count("test") != 0;
  do_test_with_delete = variables_map.count("test_with_delete") != 0;
  do_generate_chunks = variables_map.count("generate_chunks") != 0;
  do_test_store_chunk = variables_map.count("test_store_chunk") != 0;
  do_test_fetch_chunk = variables_map.count("test_fetch_chunk") != 0;
  do_test_delete_chunk = variables_map.count("test_delete_chunk") != 0;
  do_print = variables_map.count("print") != 0;

  return NoOptionsSelected() || ConflictedOptions(peer_endpoints);
}

bool SelectedOperationsContainer::ConflictedOptions(
    const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) const {
  if (!do_create && !do_load && !do_delete && !do_generate_chunks)
    return true;
  if (do_create && do_load)
    return true;
  if (do_create && do_delete)
    return true;
  if (do_load && do_delete)
    return true;
  if (do_delete && do_print)
    return true;
  if (!(do_create || do_load) && do_print)
    return true;
  if (do_bootstrap && (do_store || do_verify || do_test || do_test_with_delete))
    return true;
  if (do_bootstrap && (do_store || do_verify || do_test || do_test_with_delete))
    return true;
  if (peer_endpoints.empty() && !do_create && !do_load && !do_delete && !do_generate_chunks)
    return true;
  return false;
}

bool SelectedOperationsContainer::NoOptionsSelected() const {
  return !(do_create ||
           do_load ||
           do_bootstrap ||
           do_store ||
           do_verify ||
           do_test ||
           do_delete ||
           do_test_with_delete ||
           do_generate_chunks ||
           do_test_store_chunk ||
           do_test_fetch_chunk ||
           do_test_delete_chunk ||
           do_print);
}

Commander::Commander(size_t pmids_count)
    : pmids_count_(pmids_count),
      key_index_(pmids_count_ - 1),
      chunk_set_count_(-1),
      chunk_index_(0),
      all_keychains_(),
      keys_path_(),
      peer_endpoints_(),
      selected_ops_() {}

void Commander::AnalyseCommandLineOptions(int argc, char* argv[]) {
  po::options_description cmdline_options;
  cmdline_options.add(AddGenericOptions("Commands"))
                 .add(AddConfigurationOptions("Configuration options"));
  CheckOptionValidity(cmdline_options, argc, argv);
  ChooseOperations();
}

void Commander::GetPathFromProgramOption(const std::string& option_name,
                                         po::variables_map& variables_map) {
  if (variables_map.count(option_name) == 0)
    return;

  keys_path_ = variables_map.at(option_name).as<std::string>();
  if (keys_path_.empty())
    throw ToolsException("Incorrect information in parameter " + option_name);

  LOG(kInfo) << "GetPathFromProgramOption - " << option_name << " is " << keys_path_;
}

boost::asio::ip::udp::endpoint Commander::GetBootstrapEndpoint(const std::string& peer) {
  size_t delim = peer.rfind(':');
  boost::asio::ip::udp::endpoint ep;
  ep.port(boost::lexical_cast<uint16_t>(peer.substr(delim + 1)));
  ep.address(boost::asio::ip::address::from_string(peer.substr(0, delim)));
  LOG(kInfo) << "Going to bootstrap off endpoint " << ep;
  return ep;
}

po::options_description Commander::AddGenericOptions(const std::string& title) {
  po::options_description generic_options(title);
  generic_options.add_options()
      ("help,h", "Print this help message.")
      ("create,c", "Create keys and write to file.")
      ("load,l", "Load keys from file.")
      ("delete,d", "Delete keys file.")
      ("print,p", "Print the list of keys available.")
      ("bootstrap,b", "Run boostrap nodes only, using first 2 keys.")
      ("store,s", "Store keys on network.")
      ("verify,v", "Verify keys are available on network.")
      ("test,t", "Run simple test that stores and retrieves chunks.")
      ("test_with_delete,w", "Run simple test that stores and deletes chunks.")
      ("generate_chunks,g", "Generate a set of chunks for later on tests")
      ("test_store_chunk,1", "Run a simple test that stores a chunk from file")
      ("test_fetch_chunk,2", "Run a simple test that retrieves a chunk(recorded in file)")
      ("test_delete_chunk,3", "Run a simple test that removes a chunk(recorded in file)");
  return generic_options;
}

po::options_description Commander::AddConfigurationOptions(const std::string& title) {
  po::options_description config_file_options(title);
  boost::system::error_code error_code;
  config_file_options.add_options()
      ("peer",
       po::value<std::string>(),
       "Endpoint of bootstrap node, if attaching to running network.")
      ("pmids_count,n",
       po::value<size_t>(&pmids_count_)->default_value(pmids_count_),
       "Number of keys to create")
      ("keys_path",
       po::value<std::string>()->default_value(
           fs::path(fs::temp_directory_path(error_code) / "key_directory.dat").string()),
       "Path to keys file")
      ("chunk_path",
       po::value<std::string>()->default_value(
           fs::path(fs::temp_directory_path(error_code) / "keys_chunks").string()),
       "Path to chunk directory")
      ("key_index,k",
       po::value<size_t>(&key_index_)->default_value(key_index_),
       "The index of key to be used as client during chunk store test")
      ("chunk_set_count",
       po::value<int>(&chunk_set_count_)->default_value(chunk_set_count_),
       "Num of rounds for chunk store test, default is infinite")
      ("chunk_index",
       po::value<int>(&chunk_index_)->default_value(chunk_index_),
       "Index of the chunk to be used during tests, default is 0");
  return config_file_options;
}

void Commander::CheckOptionValidity(po::options_description& cmdline_options,
                                    int argc,
                                    char* argv[]) {
  po::command_line_parser parser(argc, argv);
  po::variables_map variables_map;
  po::store(parser.options(cmdline_options).allow_unregistered().run(), variables_map);
  po::notify(variables_map);
  GetPathFromProgramOption("keys_path", variables_map);
  if (variables_map.count("peer"))
    peer_endpoints_.push_back(GetBootstrapEndpoint(variables_map.at("peer").as<std::string>()));

  if (variables_map.count("help") || selected_ops_.InvalidOptions(variables_map, peer_endpoints_)) {
    std::cout << cmdline_options << "Options order: [c|l|d] p [b|(s|v)|t]" << std::endl;
    if (!variables_map.count("help"))
      throw ToolsException("Invalid command line options.");
  }
}

void Commander::ChooseOperations() {
  HandleKeys();
  if (selected_ops_.do_bootstrap)
    HandleSetupBootstraps();
  if (selected_ops_.do_store)
    HandleStore();
  if (selected_ops_.do_verify)
    HandleVerify();
  if (selected_ops_.do_test)
    HandleDoTest(key_index_);
  if (selected_ops_.do_test_with_delete)
    HandleDoTestWithDelete(key_index_);
  if (selected_ops_.do_generate_chunks)
    HandleGenerateChunks();
  if (selected_ops_.do_test_store_chunk)
    HandleStoreChunk(key_index_);
  if (selected_ops_.do_test_fetch_chunk)
    HandleFetchChunk(key_index_);
  if (selected_ops_.do_test_delete_chunk)
    HandleDeleteChunk(key_index_);
}

void Commander::CreateKeys() {
  all_keychains_.clear();
  for (size_t i = 0; i < pmids_count_; ++i) {
    passport::Anmaid anmaid;
    passport::Maid maid(anmaid);
    passport::Pmid pmid(maid);
    all_keychains_.push_back(passport::detail::AnmaidToPmid(anmaid, maid, pmid));
  }
  LOG(kInfo) << "Created " << all_keychains_.size() << " pmids.";
  if (maidsafe::passport::detail::WriteKeyChainList(keys_path_, all_keychains_))
    std::cout << "Wrote keys to " << keys_path_ << std::endl;
  else
    throw ToolsException("Could not write keys to " + keys_path_.string());
}

void Commander::HandleKeys() {
  if (selected_ops_.do_create) {
    CreateKeys();
  } else if (selected_ops_.do_load) {
    all_keychains_ = maidsafe::passport::detail::ReadKeyChainList(keys_path_);
    LOG(kInfo) << "Loaded " << all_keychains_.size() << " pmids from " << keys_path_;
  } else if (selected_ops_.do_delete) {
    HandleDeleteKeys();
  }

  if (selected_ops_.do_print) {
    for (size_t i(0); i < all_keychains_.size(); ++i)
      std::cout << '\t' << i
                << "\t ANMAID " << HexSubstr(all_keychains_.at(i).anmaid.name().data)
                << "\t MAID " << HexSubstr(all_keychains_.at(i).maid.name().data)
                << "\t PMID " << HexSubstr(all_keychains_.at(i).pmid.name().data)
                << (i < 2 ? " (bootstrap)" : "") << std::endl;
  }
}

void Commander::HandleSetupBootstraps() {
  assert(all_keychains_.size() >= 2);
  NetworkGenerator generator;
  generator.SetupBootstrapNodes(GetJustPmids(all_keychains_));
}

void Commander::HandleStore() {
  size_t failures(0);
  for (auto& keychain : all_keychains_) {
    try {
      KeyStorer storer(keychain, peer_endpoints_);
      storer.Store();
    }
    catch(const std::exception& e) {
      std::cout << "Failed storing key chain with PMID " << HexSubstr(keychain.pmid.name().data)
                << ": " << e.what() << std::endl;
      ++failures;
    }
  }
  if (failures > 0)
    throw ToolsException(std::string("Could not store " + std::to_string(failures) + " out of " +
                                     std::to_string(all_keychains_.size())));
}

void Commander::HandleVerify() {
  size_t failures(0);
  for (auto& keychain : all_keychains_) {
    try {
      KeyVerifier verifier(keychain, peer_endpoints_);
      verifier.Verify();
    }
    catch(const std::exception& e) {
      std::cout << "Failed verifying key chain with PMID " << HexSubstr(keychain.pmid.name().data)
                << ": " << e.what() << std::endl;
      ++failures;
    }
  }
  if (failures > 0)
    throw ToolsException(std::string("Could not verify " + std::to_string(failures) + " out of " +
                                     std::to_string(all_keychains_.size())));
}

void Commander::HandleDoTest(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_);
  chunk_storer.Test(chunk_set_count_);
}

void Commander::HandleDoTestWithDelete(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_);
  chunk_storer.TestWithDelete(chunk_set_count_);
}

void Commander::HandleStoreChunk(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_);
  chunk_storer.TestStoreChunk(chunk_index_);
}

void Commander::HandleFetchChunk(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_);
  chunk_storer.TestFetchChunk(chunk_index_);
}

void Commander::HandleDeleteChunk(size_t client_index) {
  assert(client_index > 1);
  DataChunkStorer chunk_storer(all_keychains_.at(client_index), peer_endpoints_);
  chunk_storer.TestDeleteChunk(chunk_index_);
}

void Commander::HandleGenerateChunks() {
  boost::filesystem::path  store_path(boost::filesystem::temp_directory_path() / "Chunks");
  boost::system::error_code error_code;
  if (!fs::exists(store_path, error_code)) {
    if (!fs::create_directories(store_path, error_code)) {
      LOG(kError) << "Can't create store path at " << store_path << ": " << error_code.message();
      ThrowError(CommonErrors::uninitialised);
      return;
    }
  }

  assert(chunk_set_count_ > 0);
  for (int i(0); i < chunk_set_count_; ++i) {
    ImmutableData::serialised_type content(NonEmptyString(RandomString(1 << 18)));  // 256 KB
    ImmutableData::Name name(Identity(crypto::Hash<crypto::SHA512>(content.data)));
    ImmutableData chunk_data(name, content);

    fs::path chunk_file(store_path / EncodeToBase32(chunk_data.name()->string()));
    if (!WriteFile(chunk_file, content->string()))
      LOG(kError) << "Can't store chunk " << HexSubstr(chunk_data.name()->string());
  }
}

void Commander::HandleDeleteKeys() {
  if (fs::remove(keys_path_))
    std::cout << "Deleted " << keys_path_ << std::endl;
}

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe
