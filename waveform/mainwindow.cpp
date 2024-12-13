#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>

MainWindow::
MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->SignalTreeWidget, &QTreeWidget::expanded, this,
            &MainWindow::resize_signal_tree_widget);
    connect(ui->SignalTreeWidget, &QTreeWidget::collapsed, this,
            &MainWindow::resize_signal_tree_widget);
    connect(ui->ModuleTreeWidget, &QTreeWidget::expanded, this,
            &MainWindow::resize_modul_tree_widget);
    connect(ui->ModuleTreeWidget, &QTreeWidget::collapsed, this,
            &MainWindow::resize_modul_tree_widget);
}

MainWindow::~
MainWindow()
{
    delete ui;
}
void
MainWindow::resize_signal_tree_widget() const
{
    ui->SignalTreeWidget->resizeColumnToContents(0);
}
void
MainWindow::resize_modul_tree_widget() const
{
    ui->ModuleTreeWidget->resizeColumnToContents(0);
}
