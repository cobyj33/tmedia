#ifndef TMEDIA_TMEDIA_FILE_SEARCH_H
#define TMEDIA_TMEDIA_FILE_SEARCH_H

#include <vector>
#include <filesystem>
#include <optional>

namespace tmedia {
  struct DSTOptions {
    bool ignore_images;
    bool ignore_audio;
    bool ignore_video;
    bool recurse;
  };

  enum class DSTPropValue {
    TRUE,
    FALSE,
    INHERIT
  };

  struct DSTNodeProps {
    DSTPropValue ignore_images;
    DSTPropValue ignore_audio;
    DSTPropValue ignore_video;
    DSTPropValue recurse;
  };

  class DirectorySearchTree {
    private:
      struct DirectorySearchNode {
        DirectorySearchNode* parent;
        std::string section;
        std::vector<std::unique_ptr<DirectorySearchNode>> children;
        DSTNodeProps props;
      };

      std::unique_ptr<DirectorySearchNode> root;
    
    public:
      DSTOptions options;
      DirectorySearchTree(DSTOptions options);

      bool has_path(std::filesystem::path path, DSTNodeProps props) const noexcept;
      bool remove_path(std::filesystem::path path, DSTNodeProps props);
      bool add_path(std::filesystem::path path, DSTNodeProps props);
      std::vector<std::filesystem::path> get_media_files(std::filesystem::path path, DSTNodeProps props) const;
  };
};


#endif