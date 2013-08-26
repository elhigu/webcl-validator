#ifndef WEBCLVALIDATOR_WEBCLARGUMENTS
#define WEBCLVALIDATOR_WEBCLARGUMENTS

#include <cstring>
#include <utility>
#include <vector>

/// Chains tools performing different validation stages. Links outputs
/// of earlier stages to inputs of later stages. Makes sure that tool
/// arguments match the pipeline.
///
/// \see WebCLTool
class WebCLArguments
{
public:

    /// Constructor. The command line options given by user should be
    /// passed as arguments.
    WebCLArguments(int argc, char const *argv[]);
    ~WebCLArguments();

    /// \return Number of preprocessor tool arguments.
    int getPreprocessorArgc() const;
    /// \return Preprocessor tool arguments.
    char const **getPreprocessorArgv() const;

    /// \return Number of arguments for all normalization tools.
    int getMatcherArgc() const;
    /// \return Normalization tool arguments. Input is matched with
    /// output of previous (preprocessor or normalization) tool.
    char const **getMatcherArgv() const;

    /// \return Number of arguments of memory access validation tool.
    int getValidatorArgc() const;
    /// \return Memory access validation tool arguments. Input is
    /// matched with output of previous normalization tool.
    char const **getValidatorArgv() const;

    /// \return Input file for a tool with the given
    /// options. Optionally creates an output file that becomes the
    /// input file of the next tool in the pipeline.
    char const *getInput(int argc, char const **argv, bool createOutput = false);

private:

    /// Whether there is room for input file.
    bool areArgumentsOk(int argc, char const **argv) const;

    /// \return Empty file that will be used for tool output.
    char const *createEmptyTemporaryFile(int &fd) const;
    /// \return File that has been initialized from a buffer. Allows
    /// tool to use '-include' option to include contents that have
    /// been embedded into main executable.
    char const *createFullTemporaryFile(int &fd, char const *buffer, size_t length) const;

    /// Preprocessor argument count.
    int preprocessorArgc_;
    /// Preprocessor arguments.
    char const **preprocessorArgv_;
    /// Argument count for normalization and memory access validation.
    int validatorArgc_;
    /// Arguments for normalization and memory access validation.
    char const **validatorArgv_;

    /// Pair of file descriptor and filename.
    typedef std::pair<int, char const *> TemporaryFile;
    /// Save information about temporary files so that we can close
    /// and remove them. Contains generated headers and output files.
    typedef std::vector<TemporaryFile> TemporaryFiles;
    TemporaryFiles files_;

    /// Save information about output files so that we can chain
    /// different tools properly. Contains only those files that are
    /// both outputs and inputs of adjacent tools.
    typedef std::vector<char const *> OutputFiles;
    OutputFiles outputs_;
};

#endif // WEBCLVALIDATOR_WEBCLARGUMENTS
