/*
 * ColumnClosedEvent.java
 *
 * Copyright (C) 2020 by RStudio, PBC
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

package org.rstudio.studio.client.workbench.views.source.events;

import org.rstudio.core.client.js.JavaScriptSerializable;
import org.rstudio.studio.client.application.events.CrossWindowEvent;

import com.google.gwt.event.shared.EventHandler;
import com.google.gwt.event.shared.GwtEvent;
import org.rstudio.studio.client.workbench.views.source.SourceColumn;

@JavaScriptSerializable
public class ColumnClosedEvent
   extends CrossWindowEvent<ColumnClosedEvent.Handler>
{
   public interface Handler extends EventHandler
   {
      void onColumnClosed(ColumnClosedEvent event);
   }

   public static final GwtEvent.Type<ColumnClosedEvent.Handler> TYPE =
      new GwtEvent.Type<ColumnClosedEvent.Handler>();

   public ColumnClosedEvent(SourceColumn column)
   {
      column_ = column;
   }

   public SourceColumn getColumn()
   {
      return column_;
   }

   @Override
   protected void dispatch(ColumnClosedEvent.Handler handler)
   {
      handler.onColumnClosed(this);
   }

   @Override
   public GwtEvent.Type<ColumnClosedEvent.Handler> getAssociatedType()
   {
      return TYPE;
   }

   private SourceColumn column_;
}
