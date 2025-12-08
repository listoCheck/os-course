import {Context, Next} from "koa";

export async function write(ctx: Context, next: Next) {

    await next();
}