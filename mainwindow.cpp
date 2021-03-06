#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent)
{
  QStatusBar* bottomStatusBar = new QStatusBar;

  QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
  QWidget* blnklft = new QWidget;
  QWidget* blnkrght = new QWidget;
  splitter->addWidget(blnklft);
  splitter->addWidget(blnkrght);

  QVBoxLayout* rlyt = new QVBoxLayout;
  QVBoxLayout* llyt = new QVBoxLayout;

  QGroupBox* fltrgrp = new QGroupBox("Фильтр по столбцам");
  QHBoxLayout* fltrlyt = new QHBoxLayout;
  QLineEdit* fltrline = new QLineEdit;
  QPushButton* refreshBtn = new QPushButton("Обновить таблицу");
  fltrlyt->addWidget(fltrline);
  fltrlyt->addWidget(refreshBtn);
  fltrgrp->setLayout(fltrlyt);

  QGroupBox* btnsgrp = new QGroupBox;
  QHBoxLayout* btnslyt = new QHBoxLayout;
  QPushButton* fromTable = new QPushButton("Скачать документы");
  QPushButton* intoTable = new QPushButton("Добавить документы");
  intoTable->setEnabled(false);
  fromTable->setEnabled(false);
  btnslyt->addWidget(fromTable);
  btnslyt->addWidget(intoTable);
  btnsgrp->setLayout(btnslyt);

  QTableView* viewleft = new QTableView;
  viewleft->setSortingEnabled(true);

  llyt->addWidget(fltrgrp);
  llyt->addWidget(btnsgrp);
  llyt->addWidget(viewleft);
  blnklft->setLayout(llyt);

  QTableView* viewright = new QTableView;

  rlyt->addWidget(viewright);
  blnkrght->setLayout(rlyt);

  QToolBar* toolBar = new QToolBar(this);
  QComboBox* yearsBox = new QComboBox;
  QLabel* yearsLbl= new QLabel("  Выберите год:  ");

  toolBar->addWidget(yearsLbl);
  toolBar->addWidget(yearsBox);
  toolBar->setMovable(false);
  yearsBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  setCentralWidget(splitter);
  setStatusBar(bottomStatusBar);
  addToolBar(Qt::TopToolBarArea, toolBar);
  resize(800, 600);

  if (createDbConnection())
  {
    QSqlQuery query;

    if (! query.exec("SELECT DISTINCT god FROM [budget].[dbo].[pred_null];"))
    {
      showError("Не удалось получить список годов.");
    }
    else
    {
      leftProxyModel.setSourceModel(&leftModel);

      viewleft->setModel(&leftProxyModel);
      viewright->setModel(&rightModel);

      viewleft->setSelectionMode(QAbstractItemView::SingleSelection);
      viewleft->setSelectionBehavior(QAbstractItemView::SelectRows);

      viewright->setSelectionMode(QAbstractItemView::SingleSelection);
      viewright->setSelectionBehavior(QAbstractItemView::SelectRows);

      connect(yearsBox, &QComboBox::currentTextChanged, this, &MainWindow::fillLeftModel);
      connect(viewleft, &QTableView::doubleClicked, this, &MainWindow::fillRightModel);

      connect(viewleft, &QTableView::clicked, this, &MainWindow::activateButtonsAndCurIndex);
      connect(this, &MainWindow::activateButtons, intoTable, &QPushButton::setEnabled);
      connect(this, &MainWindow::activateButtons, fromTable, &QPushButton::setEnabled);

      connect(intoTable, &QPushButton::pressed, this, &MainWindow::putDocsIntoTable);
      connect(fromTable, &QPushButton::pressed, this, &MainWindow::getDocsFromTable);

      connect(fltrline, &QLineEdit::textChanged, this, &MainWindow::setCurrentFilterString);  // заполнение строки фильтра
      connect(refreshBtn, &QPushButton::pressed, this, &MainWindow::setFilteringOnView);      // нажали на кнопку обновить таблицу

      connect(this, &MainWindow::simpleStringAndTime, bottomStatusBar, &QStatusBar::showMessage); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      while (query.next())
        yearsBox->addItem(query.value("god").toString());  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      emit this->simpleStringAndTime("Работа начата!", 0);
    }
  }
}

void MainWindow::activateButtonsAndCurIndex(const QModelIndex& index)
{
  currentLeftIndex = QPersistentModelIndex(index);  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  QModelIndex okpoIndex = leftProxyModel.index(currentLeftIndex.row(), 0); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  const QString okpo = leftProxyModel.data(okpoIndex).toString();          // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  emit this->simpleStringAndTime("Текущее ОКПО: " + okpo, 5000);

  emit this->activateButtons(true);
}

void MainWindow::setCurrentFilterString(const QString& fltrstrng)
{
  currentFilterString = fltrstrng;
}

void MainWindow::setFilteringOnView()
{
  rightModel.clear();
  leftProxyModel.setFilterWildcard(currentFilterString);

  emit this->activateButtons(false);
  emit this->simpleStringAndTime("Текущее ОКПО: сброшено!", 5000);
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void MainWindow::putDocsIntoTable()
{
  const QString rarName = QFileDialog::getOpenFileName(this, "Выберите архив с документами", "", "rar-архив (*.rar)");

  if (rarName != "")
  {
    QFileInfo rarInfo(rarName);
    const qint64 appropriateSize = 3150000; // 3MB

    if (rarInfo.size() > appropriateSize)
    {
      QMessageBox::warning(nullptr, "Предупреждение!", "Размер архива не должен превышать 3 мегабайта!");
    }
    else
    {
      QModelIndex okpoIndex = leftProxyModel.index(currentLeftIndex.row(), 0); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      const QString okpo = leftProxyModel.data(okpoIndex).toString();          // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      emit this->simpleStringAndTime("Добавляем документы для ОКПО: " + okpo, 5000);  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      QSqlQuery query;
      bool successful = query.exec("IF (SELECT COUNT(okpo) FROM budget.dbo.[pred_image] WHERE okpo=" + okpo + ")>0 "
                                    "BEGIN DELETE FROM budget.dbo.[pred_image] WHERE okpo=" + okpo + " END;");

      if (!successful)
      {
        QString errStr = query.lastError().text();
        showError("Ошибка при попытке удалить уже имеющиеся документы для данного ОКПО:\n\n" + errStr);
      }
      else
      {
        QFile rarFile(rarName);

        if (! rarFile.open(QIODevice::ReadOnly))
          showError("Не удалось открыть выбранный файл!\nЕсли для выбранного ОКПО в базе уже хранились документы, то теперь они удалены.");
        else
        {
          QByteArray fileContent = rarFile.readAll(); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

          if (fileContent.isEmpty())         // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            showError("Ошибка при чтении данных из файла! Документы не были загружены в базу.\nЕсли для выбранного ОКПО в базе уже хранились документы, то теперь они удалены.");
          else
          {
            QSqlQuery tmpQuery;
            tmpQuery.prepare("INSERT INTO budget.dbo.[pred_image] (okpo, document_org) VALUES (:okpostr, :filedata);");
            tmpQuery.bindValue(":okpostr", okpo);
            tmpQuery.bindValue(":filedata", fileContent);
            successful = tmpQuery.exec();

            if (!successful)
            {
              QString errStr = query.lastError().text();
              showError("Ошибка в запросе на добавление нового файла в базу!\nЕсли для выбранного ОКПО в базе уже хранились документы, то теперь они удалены.\n\n" + errStr);
            }
            else
              QMessageBox::information(nullptr, "Сообщение", "Файл был успешно добавлен в базу.");
          }

          rarFile.close();
        }
      }
    }
  }
}

void MainWindow::getDocsFromTable()
{
  QModelIndex okpoIndex = leftProxyModel.index(currentLeftIndex.row(), 0); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  const QString okpo = leftProxyModel.data(okpoIndex).toString();          // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  emit this->simpleStringAndTime("Скачиваем документы для ОКПО: " + okpo, 5000);  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  QSqlQuery query;
  const QString queryStr("SELECT [okpo], [document_org] FROM budget.dbo.[pred_image] WHERE okpo=" + okpo + ";");
  bool successful = query.exec(queryStr);  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  if (!successful)
  {
    QString errStr = query.lastError().text();
    showError("Не удалось получить информацию о документах для выбранного ОКПО:\n\n" + errStr);
  }
  else
  {
    bool notvisited = true;  // чтоб уж точно цикл выполнился только один раз (хотя это и будет так, т.к. в таблице отношение 1 документ к 1 окпо)

    while (query.next() && notvisited) // выполнен либо 1 раз, либо не будет выполнен вообще, если для выбранного окпо в базе нету документов!
    {
      notvisited = false;
      const QString dirName = QFileDialog::getExistingDirectory(this, "Выберите директорию для сохранения архива", "", QFileDialog::ShowDirsOnly);

      if (dirName != "")
      {
        const QString rarName = dirName + "/" + okpo + ".rar";
        QFile rarFile(rarName);

        if (rarFile.exists())
        {
          const int n = QMessageBox::warning(nullptr, "Предупреждение!", "Файл с данным именем уже существует!\n\nПерезаписать его?", QMessageBox::Yes | QMessageBox::No);

          if (n == QMessageBox::No) // если No - выйдем, иначе продолжаем
            return;
        }

        if (! rarFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
          showError("Не удалось открыть/создать файл с данным именем!");
        else
        {
          const qint64 errorFlag = rarFile.write(query.value(1).toByteArray());  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          rarFile.close();

          if (errorFlag == -1)
            showError("Ошибка при записи данных в файл!\nСодержимое файла может быть повреждено!");
          else
            QMessageBox::information(nullptr, "Сообщение", "Файл был успешно сохранён.");
        }
      }
    }

    if (notvisited)
      QMessageBox::warning(nullptr, "Предупреждение!", "База не содержит приложенные документы для выбранного ОКПО!");
  }
}

void MainWindow::fillLeftModel(const QString& curYear)
{
  if (curYear != "")
  {
    leftProxyModel.invalidate(); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    leftModel.clear();
    rightModel.clear();
    currentFilterString = "";   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    emit activateButtons(false);   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*    leftModel.setQuery("SELECT DISTINCT CAST(t1.[okpo] AS VARCHAR) AS [ОКПО] "
                           "FROM [budget].[dbo].[pred_null] t1 "
                           "WHERE god=" + curYear + ";");                                   */

    leftModel.setQuery("SELECT DISTINCT CAST(t1.[okpo] AS VARCHAR) AS [ОКПО], CAST(t3.[name] AS VARCHAR) AS [Наименование], "
                       "CAST(t3.[okato] AS VARCHAR) AS [ОКАТО], CAST(t3.[okved] AS VARCHAR) AS [ОКВЭД] "
                       "FROM [budget].[dbo].[pred_null] t1 "
                       "LEFT JOIN [gs_reg_u].dbo.[s_okpo_cur] t3 ON t1.okpo=t3.okpo "
                       "WHERE god=" + curYear + ";");

    if (leftModel.lastError().isValid())
    {
      QString errStr = leftModel.lastError().text();
      showError("Не удалось получить список организаций для выбранного года:\n\n" + errStr);
    }
  }
}

void MainWindow::fillRightModel(const QModelIndex& curIndex)
{
  QModelIndex okpoIndex = leftProxyModel.index(curIndex.row(), 0); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  QString okpo = leftProxyModel.data(okpoIndex).toString();        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


/*  rightModel.setQuery("SELECT DISTINCT [okud] AS [ОКУД], [god] AS [Год] "
                      "FROM [budget].[dbo].[pred_null] t1 "
                      "WHERE t1.okpo=" + okpo + ";");                         */


  rightModel.setQuery("SELECT DISTINCT [okud] AS [ОКУД], [god] AS [Год], [Краткое наименование формы], [Периодичность] "
                      "FROM [budget].[dbo].[pred_null] t1 "
                      "LEFT JOIN [budget].[dbo].[s_stform_all] t2 ON t2.[Код ОКУД]=t1.okud COLLATE Cyrillic_General_CI_AS "
                      "WHERE t1.okpo=" + okpo + " "
                      "ORDER BY [Периодичность], [Краткое наименование формы];");

  if (rightModel.lastError().isValid())
  {
    QString errStr = rightModel.lastError().text();
    showError("Не удалось извлечь данные для выбранного ОКПО:\n\n" + errStr);
  }
}

bool MainWindow::createDbConnection()
{
  login logData;
  readFile(logData);

  db = QSqlDatabase::addDatabase("QODBC");
  db.setDatabaseName(logData.dbName);
  db.setHostName(logData.hostName);
  db.setUserName(logData.usrName);
  db.setPassword(logData.pass);

  if (!db.open())
  {
    QString errStr = db.lastError().text();
    showError("Не удалось подключиться к базе:\n\n" + errStr);

    return false;
  }
  else
    return true;
}

void MainWindow::readFile(login& logData) const
{
  QFile inputFile("settings.conf");

  if (inputFile.open(QIODevice::ReadOnly))
  {
    QTextStream instream(&inputFile);

    while (! instream.atEnd())
    {
      QString str = instream.readLine();

      if (str.contains("Server"))
        logData.hostName = str.section(QRegExp("\\s*\\=\\s*"),-1).simplified();
      else
        if (str.contains("DataBase"))
          logData.dbName = str.section(QRegExp("\\s*\\=\\s*"),-1).simplified();
        else
          if (str.contains("User"))
            logData.usrName = str.section(QRegExp("\\s*\\=\\s*"),-1).simplified();
          else
            if (str.contains("Passwd"))
              logData.pass = str.section(QRegExp("\\s*\\=\\s*"),-1).simplified();
    }

    inputFile.close();
  }
  else
    showError("Не удалось открыть файл конфигурации.");
}

void MainWindow::showError(const QString& str) const
{
  QMessageBox* mbx = new QMessageBox(QMessageBox::Critical, "Ошибка!", str, QMessageBox::Ok);
  mbx->exec();
  delete mbx;
}
