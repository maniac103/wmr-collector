#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=wmr-forwarder
PKG_RELEASE:=3

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/wmr-forwarder
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Oregon weather station forwarder
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/wmr-forwarder/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/wmr-forwarder.config $(1)/etc/config/wmr-forwarder
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/wmr-forwarder.init $(1)/etc/init.d/wmr-forwarder
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wmr-forwarder $(1)/usr/sbin/
endef

define Package/wmr-forwarder/conffiles
/etc/config/wmr-forwarder
endef

$(eval $(call BuildPackage,wmr-forwarder))
