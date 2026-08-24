#include <QImageReader>
#include <QCoreApplication>
#include <iostream>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QImageReader reader("/home/alejandro/Escritorio/Mnemis/tests/fixtures/animated.gif");
    std::cout << "Can read: " << reader.canRead() << std::endl;
    std::cout << "Supports animation: " << reader.supportsAnimation() << std::endl;
    std::cout << "Image count: " << reader.imageCount() << std::endl;
    return 0;
}
