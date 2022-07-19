
-- --------------------------------------------------------

--
-- Table structure for table `tbl_sms`
--

CREATE TABLE `tbl_sms` (
  `id` int(255) NOT NULL,
  `date` date NOT NULL,
  `msg` longtext NOT NULL,
  `status` varchar(20) NOT NULL,
  `ddate` varchar(20) DEFAULT NULL,
  `subject` varchar(20) DEFAULT NULL,
  `to` int(11) DEFAULT NULL,
  `phone` varchar(50) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `tbl_sms`
--

INSERT INTO `tbl_sms` (`id`, `date`, `msg`, `status`, `ddate`, `subject`, `to`, `phone`) VALUES
(2, '2022-07-17', 'Dear tino,\r\nyour loan no: 121,\r\nLoan Am:30000\r\nIns No: 1\r\nIns Am:34200\r\n', 'success', NULL, 'newloan', 86, '+8801912555555');

--
-- Indexes for dumped tables
--

--
-- Indexes for table `tbl_sms`
--
ALTER TABLE `tbl_sms`
  ADD PRIMARY KEY (`id`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `tbl_sms`
--
ALTER TABLE `tbl_sms`
  MODIFY `id` int(255) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=44;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
