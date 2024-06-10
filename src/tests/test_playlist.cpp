#include <tmedia/media/playlist.h>

#include <catch2/catch_test_macros.hpp>

namespace fs = std::filesystem;

TEST_CASE("playlist", "[playlist]") {
  Playlist playlist;
  const PlaylistItem video = PlaylistItem("/home/user/.local/hidden/video.mp4");
  const PlaylistItem audio = PlaylistItem("/home/user/directory/audio.mp3");
  const PlaylistItem image = PlaylistItem("/home/user/Images/image.png");
  const PlaylistItem musicvideo = PlaylistItem("/usr/local/musicvideo.png");
  const PlaylistItem favsong = PlaylistItem("/Music/Albums/album/song.wav");

  SECTION("Initialization") {
    REQUIRE_THROWS(playlist.at(0));
    REQUIRE_THROWS(playlist.current());
    REQUIRE_THROWS(playlist.index());
    REQUIRE(playlist.size() == 0);

    REQUIRE_FALSE(playlist.can_move(PlaylistMvCmd::NEXT));
    REQUIRE_THROWS(playlist.move(PlaylistMvCmd::NEXT));

    REQUIRE_FALSE(playlist.can_move(PlaylistMvCmd::SKIP));
    REQUIRE_THROWS(playlist.move(PlaylistMvCmd::SKIP));

    REQUIRE_FALSE(playlist.can_move(PlaylistMvCmd::REWIND));
    REQUIRE_THROWS(playlist.move(PlaylistMvCmd::REWIND));

    REQUIRE_FALSE(playlist.shuffled());
    REQUIRE(playlist.loop_type() == LoopType::NO_LOOP);
  }

  SECTION("Can Move: Single Element") {
    playlist.push_back(PlaylistItem("/home/user/media.heic"));

    REQUIRE_FALSE(playlist.can_move(PlaylistMvCmd::NEXT));
    REQUIRE_THROWS(playlist.move(PlaylistMvCmd::NEXT));
    REQUIRE_FALSE(playlist.can_move(PlaylistMvCmd::SKIP));
    REQUIRE_THROWS(playlist.move(PlaylistMvCmd::SKIP));
    REQUIRE(playlist.can_move(PlaylistMvCmd::REWIND));
    REQUIRE_NOTHROW(playlist.move(PlaylistMvCmd::REWIND));
  }

  SECTION("Push back") {
    playlist.push_back(video);
    playlist.push_back(audio);

    REQUIRE(playlist.size() == 2); // video, audio
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == audio.path);

    playlist.push_back(video);
    playlist.push_back(image);
    REQUIRE(playlist.size() == 4);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == audio.path);
    REQUIRE(playlist.at(2).path == video.path);
    REQUIRE(playlist.at(3).path == image.path);

    REQUIRE(playlist.index() == 0);
    REQUIRE(playlist.current().path == video.path);
  }

  SECTION("Empty Insert") {
    REQUIRE_NOTHROW(playlist.insert(image, 0));
    REQUIRE(playlist.size() == 1);
    REQUIRE(playlist.at(0).path == image.path);
    REQUIRE(playlist.current().path == image.path);
    REQUIRE(playlist.index() == 0);
  }

  SECTION("Insertion Order") {
    playlist.push_back(video);
    playlist.push_back(audio);
    playlist.insert(image, 1); // video, image, audio

    REQUIRE(playlist.size() == 3);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == image.path);
    REQUIRE(playlist.at(2).path == audio.path);
    playlist.insert(image, 1); // video, image, image, audio

    REQUIRE(playlist.size() == 4);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == image.path);
    REQUIRE(playlist.at(2).path == image.path);
    REQUIRE(playlist.at(3).path == audio.path);
    playlist.insert(image, 1); // video, image, image, image, audio

    REQUIRE(playlist.size() == 5);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == image.path);
    REQUIRE(playlist.at(2).path == image.path);
    REQUIRE(playlist.at(3).path == image.path);
    REQUIRE(playlist.at(4).path == audio.path);

    // backinsert
    playlist.insert(favsong, playlist.size()); // video, image, image, image, audio, favsong
    REQUIRE(playlist.size() == 6);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == image.path);
    REQUIRE(playlist.at(2).path == image.path);
    REQUIRE(playlist.at(3).path == image.path);
    REQUIRE(playlist.at(4).path == audio.path);
    REQUIRE(playlist.at(5).path == favsong.path);

    SECTION("Deletion") {
      playlist.remove(0); // image, image, image, audio, favsong
      REQUIRE(playlist.size() == 5);
      REQUIRE(playlist.at(0).path == image.path);

      playlist.remove(3); // image, image, image, favsong
      REQUIRE(playlist.size() == 4);
      REQUIRE(playlist.at(0).path == image.path);
      REQUIRE(playlist.at(1).path == image.path);
      REQUIRE(playlist.at(2).path == image.path);
      REQUIRE(playlist.at(3).path == favsong.path);

      playlist.clear();
      REQUIRE(playlist.size() == 0);
      REQUIRE_THROWS(playlist.at(0));
    }
  }

  SECTION("trivial has") {
    playlist.push_back(favsong);
    REQUIRE(playlist.has(favsong));
    playlist.remove(0);
    REQUIRE_FALSE(playlist.has(favsong));
  }

  SECTION("Empty Remove") {
    REQUIRE_NOTHROW(playlist.remove(image));
  }

  SECTION("Remove First") {
    playlist.push_back(video);
    playlist.push_back(image);
    playlist.push_back(audio);
    playlist.push_back(image);
    playlist.remove(image);

    REQUIRE(playlist.size() == 3);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == audio.path);
    REQUIRE(playlist.at(2).path == image.path);

    playlist.remove(image);

    REQUIRE(playlist.size() == 2);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == audio.path);

    playlist.remove(image); // no-op

    REQUIRE(playlist.size() == 2);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == audio.path);
  }

  SECTION("Clear") {
    playlist.push_back(video);
    playlist.push_back(audio);
    playlist.push_back(image);

    playlist.clear();

    REQUIRE(playlist.empty());
    REQUIRE(playlist.size() == 0);
  }

  SECTION("Entry Remove") {
    playlist.push_back(favsong); // favsong
    playlist.push_back(video); // favsong, video
    playlist.push_back(audio); // favsong, video, audio
    playlist.push_back(image); // favsong, video, audio, image

    playlist.remove(favsong); // video, audio, image
    REQUIRE(playlist.size() == 3);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == audio.path);
    REQUIRE(playlist.at(2).path == image.path);

    playlist.remove(audio); // video, image
    REQUIRE(playlist.size() == 2);
    REQUIRE(playlist.at(0).path == video.path);
    REQUIRE(playlist.at(1).path == image.path);

    playlist.remove(image); // video
    REQUIRE(playlist.size() == 1);
    REQUIRE(playlist.at(0).path == video.path);

    playlist.remove(image); // video (no-op)
    REQUIRE(playlist.size() == 1);
    REQUIRE(playlist.at(0).path == video.path);

    playlist.remove(video);
    REQUIRE(playlist.size() == 0);
  }

}
