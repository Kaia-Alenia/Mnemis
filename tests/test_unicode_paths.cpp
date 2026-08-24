#include <gtest/gtest.h>
#include "filesystem/StdFileSystem.hpp"
#include <QTemporaryDir>
#include <QFile>
#include <QString>
#include <string>

using namespace mnemis::filesystem;

class DummyLogger : public mnemis::core::ILogger {
public:
    void log(mnemis::core::LogLevel level, std::string_view message) override {}
};

TEST(UnicodePathsTest, StdFileSystemHandlesUnicode) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());
    
    // Create a path with Unicode characters
    QString unicodePath = tempDir.path() + "/test_áéíóú_ñ_日本語.txt";
    QFile file(unicodePath);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("test");
    file.close();
    
    DummyLogger logger;
    StdFileSystem fs(&logger);
    
    // Check if StdFileSystem correctly handles the unicode string path
    std::string stdPath = unicodePath.toStdString();
    
    auto acc = fs.isAccessible(stdPath);
    EXPECT_TRUE(acc.isSuccess() && acc.value());
    auto idRes = fs.getFileIdentity(stdPath);
    ASSERT_TRUE(idRes.isSuccess());
    EXPECT_EQ(idRes.value().fileSize, 4);
}
