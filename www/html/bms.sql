-- MySQL dump 10.13  Distrib 8.0.42, for Linux (x86_64)
--
-- Host: localhost    Database: bms
-- ------------------------------------------------------
-- Server version	8.0.42-0ubuntu0.24.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `balance`
--

DROP TABLE IF EXISTS `balance`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `balance` (
  `AccNo` int NOT NULL,
  `Balance` decimal(15,0) NOT NULL DEFAULT '0',
  `Interest` decimal(15,0) NOT NULL DEFAULT '0',
  PRIMARY KEY (`AccNo`),
  CONSTRAINT `balance_ibfk_1` FOREIGN KEY (`AccNo`) REFERENCES `credentials` (`AccNo`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `balance`
--

LOCK TABLES `balance` WRITE;
/*!40000 ALTER TABLE `balance` DISABLE KEYS */;
INSERT INTO `balance` VALUES (197,35,2),(198,5,0),(199,45,2),(200,207,12),(201,0,0),(202,0,0),(203,0,0),(204,0,0),(205,0,0),(206,0,0),(207,400,0),(208,1100,0),(209,4500,0),(210,475,0),(211,612,0),(212,500,0),(213,500,0);
/*!40000 ALTER TABLE `balance` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `credentials`
--

DROP TABLE IF EXISTS `credentials`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `credentials` (
  `AccNo` int NOT NULL AUTO_INCREMENT,
  `Pass` varchar(100) NOT NULL,
  PRIMARY KEY (`AccNo`)
) ENGINE=InnoDB AUTO_INCREMENT=214 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `credentials`
--

LOCK TABLES `credentials` WRITE;
/*!40000 ALTER TABLE `credentials` DISABLE KEYS */;
INSERT INTO `credentials` VALUES (197,'SHA256_OF_pass197'),(198,'SHA256_OF_pass198'),(199,'SHA256_OF_pass199'),(200,'SHA256_OF_pass200'),(201,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f'),(202,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f'),(203,'1ea2f89d934cb4a2af0b486736609cf9cb4bdafdc6e946e79aecb02b9d9dceb4'),(204,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f'),(205,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f'),(206,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f'),(207,'4a9ca4596692e94f9d2912b06a0d007564a22ee750339a6021c2392149b25d6d'),(208,'ba28e715bb87f8126262007fd9651d3cae6a60faa48ccf8b62124b0fb7f6e01b'),(209,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f'),(210,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f'),(211,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f'),(212,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f'),(213,'ef797c8118f02dfb649607dd5d3f8c7623048c9c063d532cc95c5ed7a898a64f');
/*!40000 ALTER TABLE `credentials` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `transactions`
--

DROP TABLE IF EXISTS `transactions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `transactions` (
  `Sender` int DEFAULT NULL,
  `Receiver` int NOT NULL,
  `Amount` decimal(10,0) NOT NULL,
  `Remarks` varchar(50) NOT NULL,
  `DateTime` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `SenBalance` decimal(10,0) NOT NULL,
  `RecBalance` decimal(10,0) NOT NULL,
  `TransactionID` int NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`TransactionID`)
) ENGINE=InnoDB AUTO_INCREMENT=25 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `transactions`
--

LOCK TABLES `transactions` WRITE;
/*!40000 ALTER TABLE `transactions` DISABLE KEYS */;
INSERT INTO `transactions` VALUES (200,197,10,'Hired as Accountant','2024-12-19 15:46:17',59,79,1),(200,198,10,'Hired as Sales Manager','2024-12-19 15:47:02',49,79,2),(200,199,30,'Hired as Operation Manager','2024-12-19 15:47:37',19,99,3),(200,199,19,'Investment','2024-12-19 15:48:40',0,118,4),(197,200,10,'First Income','2024-12-19 15:49:17',69,10,5),(198,197,40,'ROI Again','2024-12-19 15:50:00',39,109,6),(198,197,30,'Miscellaneous Costs','2024-12-19 15:50:22',9,139,7),(199,197,30,'Sales Income','2024-12-19 15:50:47',88,169,8),(199,200,20,'Advetisers Payout','2024-12-19 15:51:14',68,30,9),(197,200,100,'Regular Income now','2024-12-19 15:51:36',69,130,10),(197,200,50,'More Sales','2024-12-19 15:51:55',19,180,11),(199,200,30,'Providers Funding','2024-12-19 15:52:40',38,210,12),(200,197,10,'Tax Cuts','2024-12-19 15:54:24',200,29,13),(198,197,4,'All remaining Bal','2024-12-19 22:13:47',5,33,14),(200,199,5,'Turn Over Profits','2024-12-19 09:21:59',207,45,15),(0,208,1000,'Signup Bonus','2025-08-10 16:32:32',0,1000,16),(208,207,400,'','2025-08-10 16:44:42',600,400,17),(0,209,5000,'Signup Bonus','2025-08-10 16:45:42',0,5000,18),(209,208,500,'','2025-08-10 16:46:19',4500,1100,19),(0,210,520,'Signup Bonus','2025-08-10 16:57:19',0,520,20),(0,211,567,'Signup Bonus','2025-08-10 16:57:51',0,567,21),(210,211,45,'','2025-08-10 16:58:23',475,612,22),(0,212,500,'Signup Bonus','2025-08-10 18:59:02',0,500,23),(0,213,500,'Signup Bonus','2025-08-10 19:03:32',0,500,24);
/*!40000 ALTER TABLE `transactions` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `userinfo`
--

DROP TABLE IF EXISTS `userinfo`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `userinfo` (
  `AccNo` int NOT NULL,
  `Name` varchar(50) DEFAULT NULL,
  `Address` varchar(100) DEFAULT NULL,
  `Email` varchar(64) DEFAULT NULL,
  PRIMARY KEY (`AccNo`),
  CONSTRAINT `userinfo_ibfk_1` FOREIGN KEY (`AccNo`) REFERENCES `credentials` (`AccNo`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `userinfo`
--

LOCK TABLES `userinfo` WRITE;
/*!40000 ALTER TABLE `userinfo` DISABLE KEYS */;
INSERT INTO `userinfo` VALUES (197,'Ram Bahadur','Syanjga','ram@bahadur.com'),(198,'Jaja Bahadur','Jhapa','haha@bahadur.com'),(199,'Dam Bahadur','Dhanghadi','dam@bahadur.com'),(200,'Sangam Adhikari','Pokhara','sangam@adhikari.com'),(201,'Hemendu Kumar','Patna','kumar@gmail.com'),(202,'Hemendu Kumar','Patna','kumar@gmail.com'),(203,'ljldlksaj','kjsdlf','sfsfss@gmail.com'),(204,'Hemendu kumar','Patna','kumar@gmail.com'),(205,'Hemendu Kumar','Patna','kumar@gmail.com'),(206,'Shubham Kumar','Patna','Kumarshubham@gmail.com'),(207,'Rishav Kumar','Patna','kumarrishav@gmail.com'),(208,'Anshu','kolkata','anshu@gmail.com'),(209,'Rishav','Patna','kumar@gmail.com'),(210,'Rishav','patna','kumar@gmail.com'),(211,'shivu','patna','kumar@gmail.com'),(212,'Hemendu Kumar','Patna','kumar@gmail.com'),(213,'shubham kumar','patna','kumar@gmail.com');
/*!40000 ALTER TABLE `userinfo` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2025-08-10 20:04:15