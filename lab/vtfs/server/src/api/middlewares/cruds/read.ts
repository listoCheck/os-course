import {Context, Next} from "koa";

export async function read(ctx: Context, next: Next) {

    await next();
}