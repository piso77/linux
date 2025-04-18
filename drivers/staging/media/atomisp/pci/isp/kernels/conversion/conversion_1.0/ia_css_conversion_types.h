/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Support for Intel Camera Imaging ISP subsystem.
 * Copyright (c) 2015, Intel Corporation.
 */

#ifndef __IA_CSS_CONVERSION_TYPES_H
#define __IA_CSS_CONVERSION_TYPES_H

/**
 *  Conversion Kernel parameters.
 *  Deinterleave bayer quad into isys format
 *
 *  ISP block: CONVERSION
 *
 */
struct ia_css_conversion_config {
	u32 en;     /** en parameter */
	u32 dummy0; /** dummy0 dummy parameter 0 */
	u32 dummy1; /** dummy1 dummy parameter 1 */
	u32 dummy2; /** dummy2 dummy parameter 2 */
};

#endif /* __IA_CSS_CONVERSION_TYPES_H */
