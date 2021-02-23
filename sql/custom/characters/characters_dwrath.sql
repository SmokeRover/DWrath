--Creates basic table for dwrath unique database entires--
CREATE TABLE IF NOT EXISTS `custom_dwrath_character_stats` (
	`GUID` TINYINT(3) UNSIGNED NOT NULL,
	`Difficulty` FLOAT NOT NULL DEFAULT '0',
	`GroupSize` INT(11) NOT NULL DEFAULT '1',
	`SpellPower` INT(10) UNSIGNED NOT NULL DEFAULT '0',
	`Stats` FLOAT NOT NULL DEFAULT '100',
	`GroupLevelTog` TINYINT(3) NULL DEFAULT '0',
	PRIMARY KEY (`GUID`) USING BTREE
)
DEFAULT CHARSET=utf8
COLLATE='utf8_general_ci'
ENGINE=InnoDB
;
