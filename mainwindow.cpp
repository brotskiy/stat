#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent)
{
  QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

  QWidget* blnklft = new QWidget;
  QWidget* blnkrght = new QWidget;

  splitter->addWidget(blnklft);
  splitter->addWidget(blnkrght);

  QVBoxLayout* rlyt = new QVBoxLayout;
  QVBoxLayout* llyt = new QVBoxLayout;

  QLineEdit* fltrline = new QLineEdit;
  QTableView* viewleft = new QTableView;
  QPushButton* fromTable = new QPushButton("Скачать документы");
  QPushButton* intoTable = new QPushButton("Добавить документы");
  intoTable->setEnabled(false);
  fromTable->setEnabled(false);

  llyt->addWidget(fltrline);
  llyt->addWidget(fromTable);
  llyt->addWidget(intoTable);
  llyt->addWidget(viewleft);
  blnklft->setLayout(llyt);

  QTableView* viewright = new QTableView;

  rlyt->addWidget(viewright);
  blnkrght->setLayout(rlyt);

  QToolBar* toolBar = new QToolBar(this);
  QComboBox* yearsBox = new QComboBox(toolBar);

  toolBar->setMovable(false);
  toolBar->addWidget(yearsBox);
  yearsBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  setCentralWidget(splitter);
  addToolBar(Qt::TopToolBarArea, toolBar);
  resize(800, 600);

  if (createDbConnection())
  {
    QSqlQuery query; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if (! query.exec("SELECT DISTINCT god FROM [budget].[dbo].[pred_null];"))  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    {
      showError("Не удалось получить список годов.");
    }
    else
    {
      leftProxyModel.setSourceModel(&leftModel);

      viewleft->setModel(&leftProxyModel);
      viewright->setModel(&rightModel);

      viewleft->setSelectionMode(QAbstractItemView::SingleSelection); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      viewleft->setSelectionBehavior(QAbstractItemView::SelectRows);

      viewright->setSelectionMode(QAbstractItemView::SingleSelection); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      viewright->setSelectionBehavior(QAbstractItemView::SelectRows);

      //viewright->setSelectionMode(QAbstractItemView::NoSelection);

      connect(yearsBox, &QComboBox::currentTextChanged, this, &MainWindow::fillLeftModel);
      connect(viewleft, &QTableView::doubleClicked, this, &MainWindow::fillRightModel);

      connect(viewleft, &QTableView::clicked, this, &MainWindow::actButtons);  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      connect(this, &MainWindow::activateButtons, intoTable, &QPushButton::setEnabled); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      connect(this, &MainWindow::activateButtons, fromTable, &QPushButton::setEnabled);

      connect(intoTable, &QPushButton::pressed, this, &MainWindow::putDocsIntoTable);  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      connect(fromTable, &QPushButton::pressed, this, &MainWindow::getDocsFromTable);

      connect(viewleft->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::setCurrentLeftIndex); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      connect(fltrline, &QLineEdit::textChanged, &leftProxyModel, &QSortFilterProxyModel::setFilterWildcard);  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      while (query.next())
        yearsBox->addItem(query.value("god").toString());  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
  }
}

void MainWindow::actButtons(const QModelIndex& index)
{
  emit this->activateButtons(true);
}

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
      QModelIndex okpoIndex = leftModel.index(currentLeftIndex.row(), 0); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      const QString okpo = leftModel.data(okpoIndex).toString();

      QSqlQuery query;
      bool successful = query.exec("IF (SELECT COUNT(okpo) FROM budget.dbo.[pred_image] WHERE okpo=" + okpo + ")>0 "
                                   "BEGIN DELETE FROM budget.dbo.[pred_image] WHERE okpo=" + okpo + " END");             // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      if (!successful)
      {
        QString errStr = query.lastError().text();
        showError("Ошибка при попытке удалить уже имеющиеся документы для данного ОКПО:\n\n" + errStr);
      }
      else
      {
        successful = query.exec("INSERT INTO pred_image(okpo,document_org) SELECT " + okpo + ",BulkColumn from OpenRowSet"
                                "( BULK N\"" + rarName + "\",Single_BLOB) as файл");                                        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        if (!successful)
        {
          QString errStr = query.lastError().text();
          showError("Не удалось загрузить новый файл в базу.\nЕсли для выбранного ОКПО в базе уже хранились документы, то теперь они удалены.\n\n" + errStr);
        }
      }
    }
  }
}

void MainWindow::getDocsFromTable()
{
  QModelIndex okpoIndex = leftModel.index(currentLeftIndex.row(), 0); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  const QString okpo = leftModel.data(okpoIndex).toString();

  QSqlQuery query;
  bool successful = query.exec("SELECT DISTINCT okpo FROM budget.dbo.[pred_image] WHERE okpo=" + okpo + ";");  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  if (!successful)
  {
    QString errStr = query.lastError().text();
    showError("Не удалось получить информацию о документах для выбранного ОКПО:\n\n" + errStr);
  }
  else
  {
    bool notvisited = true;

    while (query.next()) // из-за distinct будет выполнен либо 1 раз, либо не будет выполнен вообще, если для выбранного окпо в базе нету документов!
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

        successful = query.exec("SELECT DISTINCT document_org FROM budget.dbo.[pred_image] WHERE okpo=" + okpo + ";"); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        if (!successful)
        {
          QString errStr = query.lastError().text();
          showError("Не удалось извлечь документы для данного ОКПО из базы:\n\n" + errStr);
        }
        else
        {
          if (! rarFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
          {
            showError("Не открыть/создать файл с данным именем!");
          }
          else
          {
            query.next(); // чтобы стать на первую и единственную строку запроса

            QDataStream stream(&rarFile);
            stream << query.value(0).toByteArray(); // можно попробовать writeRawData!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

            if (stream.status() != QDataStream::Ok)
              showError("Ошибка при записи данных из запроса в файл!\nСодержимое файла может быть повреждено!");

            rarFile.close();
          }
        }
      }
    }

    if (notvisited)
      QMessageBox::warning(nullptr, "Предупреждение!", "База не содержит приложенные документы для выбранного ОКПО!");
  }
}

void MainWindow::setCurrentLeftIndex(const QModelIndex &current, const QModelIndex &previous)
{
  currentLeftIndex = current;
}

void MainWindow::fillLeftModel(const QString& curYear)
{
  if (curYear != "")
  {
    leftModel.clear();
    rightModel.clear();

    emit activateButtons(false); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    leftModel.setQuery("SELECT DISTINCT CAST(t1.[okpo] AS VARCHAR) AS [ОКПО], t3.[name] AS [Наименование], CAST(t3.[okato] AS VARCHAR) AS [ОКАТО], t3.okved AS [ОКВЭД] "
                       "FROM [budget].[dbo].[pred_null] t1 "
                       "LEFT JOIN [gs_reg_u].dbo.[s_okpo_cur] t3 ON t1.okpo=t3.okpo "
                       "WHERE god=" + curYear + " "                                                 // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                       "ORDER BY [ОКПО];");

    if (leftModel.lastError().isValid())
    {
      QString errStr = leftModel.lastError().text();
      showError("Не удалось получить список организаций для выбранного года:\n\n" + errStr);
    }
  }
}

void MainWindow::fillRightModel(const QModelIndex& curIndex)
{
  QModelIndex okpoIndex = leftModel.index(curIndex.row(), 0); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  QString okpo = leftModel.data(okpoIndex).toString();

  rightModel.setQuery("SELECT DISTINCT [okud] AS [ОКУД], [god] AS [Год], [Краткое наименование формы], [Периодичность] "
                      "FROM [budget].[dbo].[pred_null] t1 "
                      "LEFT JOIN [budget].[dbo].[s_stform_all] t2 ON t2.[Код ОКУД]=t1.okud COLLATE Cyrillic_General_CI_AS "
                      "WHERE t1.okpo=" + okpo + " "                                                                            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
  db.setDatabaseName(logData.dbName);       // !!!!!!!!!!!!!!
  db.setHostName(logData.hostName);         // !!!!!!!!!!!!!!
  db.setUserName(logData.usrName);          // !!!!!!!!!!!!!!
  db.setPassword(logData.pass);             // !!!!!!!!!!!!!!

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
        logData.hostName = str.section(QRegExp("\\s*\\=\\s*"),-1).simplified(); // !!!!!!!!!!!!!!!!!!!!!
      else
        if (str.contains("DataBase"))
          logData.dbName = str.section(QRegExp("\\s*\\=\\s*"),-1).simplified(); // !!!!!!!!!!!!!!!!!!!!!
        else
          if (str.contains("User"))
            logData.usrName = str.section(QRegExp("\\s*\\=\\s*"),-1).simplified(); // !!!!!!!!!!!!!!!!!!!!!
          else
            if (str.contains("Passwd"))
              logData.pass = str.section(QRegExp("\\s*\\=\\s*"),-1).simplified(); // !!!!!!!!!!!!!!!!!!!!!
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
