#include <QApplication>

int run_notebook_tests(int argc, char **argv);
int run_navigation_tests(int argc, char **argv);
int run_settings_tests(int argc, char **argv);
int run_findinfiles_tests(int argc, char **argv);

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    int status = 0;
    status |= run_notebook_tests(argc, argv);
    status |= run_navigation_tests(argc, argv);
    status |= run_settings_tests(argc, argv);
    status |= run_findinfiles_tests(argc, argv);
    return status;
}
