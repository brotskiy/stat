#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QtSql>
#include <QFile>

class MainWindow : public QMainWindow
{
    Q_OBJECT

    struct login
    {
        QString hostName, dbName, usrName, pass;
    };

    QSqlDatabase db;
    QSqlQueryModel leftModel;
    QSortFilterProxyModel leftProxyModel;
    QSqlQueryModel rightModel;

    QModelIndex currentLeftIndex;

    bool createDbConnection();
    void readFile(login& logData) const;
    void showError(const QString& str) const;

  public:
    MainWindow(QWidget *parent = nullptr);

  private slots:
    void fillLeftModel(const QString& str);
    void fillRightModel(const QModelIndex& curIndex);
    void putDocsIntoTable();
    void getDocsFromTable();

    void setCurrentLeftIndex(const QModelIndex &current, const QModelIndex &previous);

    void actButtons(const QModelIndex& index); // !!!!!!!!!!!!!!!

  signals:
    void activateButtons(bool flag); // !!!!!!!!!!!!!!!!!!!!!!!!!
};

#endif // MAINWINDOW_H
